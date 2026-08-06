#!/usr/bin/env python3
"""Correlate IL2TEX no-upload textures with IL2ALIAS placed-resource ranges."""

from __future__ import annotations

import argparse
import collections
import gzip
import pathlib
import re
from dataclasses import dataclass
from typing import Any, Iterable, TextIO


TEXTURE_EVENT_RE = re.compile(
    r"IL2TEX (create|srv|copy_resource|copy|destroy|suppressed)\s+(.*)$"
)
ALIAS_EVENT_RE = re.compile(
    r"IL2ALIAS (create|destroy|barrier|suppressed)\s+(.*)$"
)
FIELD_RE = re.compile(r"([a-z_]+)=([^\s]+)")
BLOCK_COMPRESSED_COLOR_FORMATS = {
    71, 72, 74, 75, 77, 78, 80, 81, 83, 84, 95, 96, 98, 99,
}
FORMAT_NAMES = {
    71: "BC1_UNORM",
    72: "BC1_UNORM_SRGB",
    77: "BC3_UNORM",
    78: "BC3_UNORM_SRGB",
    98: "BC7_UNORM",
    99: "BC7_UNORM_SRGB",
}
DIMENSION_NAMES = {
    1: "BUFFER",
    2: "TEXTURE1D",
    3: "TEXTURE2D",
    4: "TEXTURE3D",
}


@dataclass
class PlacedResource:
    cookie: int
    fields: dict[str, Any]
    create_line: int
    destroy_line: int | None = None

    @property
    def heap(self) -> str:
        return str(self.fields.get("heap", "(nil)"))

    @property
    def offset(self) -> int:
        return int(self.fields.get("offset", 0))

    @property
    def size(self) -> int:
        return int(self.fields.get("size", 0))

    @property
    def end(self) -> int:
        return self.offset + self.size

    @property
    def kind(self) -> str:
        return str(self.fields.get("kind", "unknown"))


def parse_value(value: str) -> Any:
    value = value.rstrip(".")
    try:
        if value.startswith(("0x", "0X")):
            return int(value, 16)
        if re.fullmatch(r"-?\d+", value):
            return int(value)
        if re.fullmatch(r"-?(?:\d+\.\d*|\d*\.\d+)", value):
            return float(value)
    except ValueError:
        pass
    return value


def parse_fields(text: str) -> dict[str, Any]:
    return {name: parse_value(value) for name, value in FIELD_RE.findall(text)}


def open_log(path: pathlib.Path) -> TextIO:
    if path.suffix == ".gz":
        return gzip.open(path, "rt", encoding="utf-8", errors="replace")
    return path.open("r", encoding="utf-8", errors="replace")


def ranges_overlap(a: PlacedResource, b: PlacedResource) -> bool:
    return a.heap == b.heap and a.offset < b.end and b.offset < a.end


def lifetimes_overlap(a: PlacedResource, b: PlacedResource) -> bool:
    a_end = a.destroy_line if a.destroy_line is not None else 2**63
    b_end = b.destroy_line if b.destroy_line is not None else 2**63
    return a.create_line < b_end and b.create_line < a_end


def format_name(value: Any) -> str:
    if isinstance(value, int):
        return FORMAT_NAMES.get(value, f"DXGI#{value}")
    return str(value)


def format_shape(fields: dict[str, Any]) -> str:
    dimension = DIMENSION_NAMES.get(fields.get("dimension"), f"DIM#{fields.get('dimension', '?')}")
    return (
        f"{dimension} {fields.get('width', '?')}x{fields.get('height', '?')}x"
        f"{fields.get('depth_or_layers', '?')}; mips={fields.get('mips', '?')}; "
        f"fmt={format_name(fields.get('format', '?'))}"
    )


def markdown_table(headers: list[str], rows: Iterable[Iterable[Any]]) -> list[str]:
    output = ["| " + " | ".join(headers) + " |"]
    output.append("|" + "|".join("---" for _ in headers) + "|")
    for row in rows:
        output.append("| " + " | ".join(str(value) for value in row) + " |")
    return output


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("log", type=pathlib.Path)
    parser.add_argument("--output", type=pathlib.Path, required=True)
    args = parser.parse_args()

    alias_counts: collections.Counter[str] = collections.Counter()
    texture_counts: collections.Counter[str] = collections.Counter()
    texture_creates: dict[int, dict[str, Any]] = {}
    texture_activity: dict[int, collections.Counter[str]] = collections.defaultdict(collections.Counter)
    resources: dict[int, PlacedResource] = {}
    pending_destroys: dict[int, int] = {}
    barriers: list[tuple[int, dict[str, Any]]] = []

    with open_log(args.log) as stream:
        for line_number, line in enumerate(stream, 1):
            texture_match = TEXTURE_EVENT_RE.search(line)
            if texture_match:
                event = texture_match.group(1)
                fields = parse_fields(texture_match.group(2))
                texture_counts[event] += 1
                if event == "create":
                    texture_creates[int(fields["cookie"])] = fields
                elif event == "srv":
                    texture_activity[int(fields["cookie"])]["srv"] += 1
                elif event in ("copy", "copy_resource"):
                    texture_activity[int(fields["src_cookie"])]["copy_src"] += 1
                    texture_activity[int(fields["dst_cookie"])]["copy_dst"] += 1

            alias_match = ALIAS_EVENT_RE.search(line)
            if not alias_match:
                continue
            event = alias_match.group(1)
            fields = parse_fields(alias_match.group(2))
            alias_counts[event] += 1
            if event == "create":
                cookie = int(fields["cookie"])
                resources[cookie] = PlacedResource(
                    cookie=cookie,
                    fields=fields,
                    create_line=line_number,
                    destroy_line=pending_destroys.pop(cookie, None),
                )
            elif event == "destroy":
                cookie = int(fields["cookie"])
                if cookie in resources:
                    resources[cookie].destroy_line = line_number
                else:
                    pending_destroys[cookie] = line_number
            elif event == "barrier":
                barriers.append((line_number, fields))

    target_cookies = {
        cookie for cookie, fields in texture_creates.items()
        if fields.get("format") in BLOCK_COMPRESSED_COLOR_FORMATS
        and int(fields.get("mips", 1)) > 1
        and texture_activity[cookie]["srv"] > 0
        and texture_activity[cookie]["copy_dst"] == 0
    }
    traced_targets = [resources[cookie] for cookie in target_cookies if cookie in resources]
    missing_target_creates = target_cookies - resources.keys()

    by_heap: dict[str, list[PlacedResource]] = collections.defaultdict(list)
    for resource in resources.values():
        by_heap[resource.heap].append(resource)

    barriers_by_cookie: dict[int, list[tuple[int, dict[str, Any]]]] = collections.defaultdict(list)
    barrier_kinds: collections.Counter[tuple[str, str]] = collections.Counter()
    for line_number, barrier in barriers:
        before_cookie = int(barrier.get("before_cookie", 0))
        after_cookie = int(barrier.get("after_cookie", 0))
        barrier_kinds[(str(barrier.get("before_kind")), str(barrier.get("after_kind")))] += 1
        if before_cookie:
            barriers_by_cookie[before_cookie].append((line_number, barrier))
        if after_cookie and after_cookie != before_cookie:
            barriers_by_cookie[after_cookie].append((line_number, barrier))

    target_classification: collections.Counter[str] = collections.Counter()
    target_rows: list[tuple[Any, ...]] = []
    target_barrier_count = 0
    targets_with_any_range_overlap = 0
    targets_with_live_buffer_overlap = 0
    targets_with_live_texture_overlap = 0

    for target in traced_targets:
        overlapping = [
            other for other in by_heap[target.heap]
            if other.cookie != target.cookie and ranges_overlap(target, other)
        ]
        live_overlapping = [other for other in overlapping if lifetimes_overlap(target, other)]
        live_buffers = [other for other in live_overlapping if other.kind == "buffer"]
        live_textures = [other for other in live_overlapping if other.kind == "texture"]
        target_barriers = barriers_by_cookie.get(target.cookie, [])

        if overlapping:
            targets_with_any_range_overlap += 1
        if live_buffers:
            targets_with_live_buffer_overlap += 1
        if live_textures:
            targets_with_live_texture_overlap += 1
        if target_barriers:
            target_barrier_count += 1

        if live_buffers:
            target_classification["live buffer range overlap"] += 1
        elif live_textures:
            target_classification["live texture-only range overlap"] += 1
        elif overlapping:
            target_classification["range reused outside overlapping lifetime"] += 1
        else:
            target_classification["no traced placed-resource range overlap"] += 1

        target_rows.append(
            (
                target.cookie,
                format_shape(target.fields),
                target.heap,
                target.offset,
                target.size,
                len(overlapping),
                len(live_buffers),
                len(live_textures),
                len(target_barriers),
            )
        )

    target_rows.sort(key=lambda row: (row[8], row[6], row[7], row[5]), reverse=True)
    resource_kinds = collections.Counter(resource.kind for resource in resources.values())

    output: list[str] = [
        "# Placed-resource alias trace analysis",
        "",
        "This report correlates D02 no-incoming-copy SRV textures with D03 placed-resource "
        "ranges and explicit D3D12 alias barriers. Range overlap is evidence of shared heap "
        "address space, not by itself evidence of invalid aliasing or a translation defect.",
        "",
        "## Trace validity and census",
        "",
    ]
    output.extend(markdown_table(
        ["event family", "event", "count"],
        [
            *(('IL2ALIAS', event, alias_counts[event]) for event in ('create', 'destroy', 'barrier', 'suppressed')),
            *(('IL2TEX', event, texture_counts[event]) for event in ('create', 'srv', 'copy', 'copy_resource', 'destroy', 'suppressed')),
        ],
    ))
    output.extend([
        "",
        f"Placed resources: {len(resources)} across {len(by_heap)} heaps "
        f"({resource_kinds.get('buffer', 0)} buffers, {resource_kinds.get('texture', 0)} textures).",
        "",
        "## D02 no-incoming-copy SRV class",
        "",
        f"Candidate textures from the same run: {len(target_cookies)}; candidates with a "
        f"matching IL2ALIAS placed-resource record: {len(traced_targets)}; missing: "
        f"{len(missing_target_creates)}.",
        "",
    ])
    output.extend(markdown_table(
        ["classification", "target textures"], target_classification.most_common()
    ))
    output.extend([
        "",
        f"Targets with any same-heap range overlap: {targets_with_any_range_overlap}; "
        f"with a lifetime-overlapping buffer: {targets_with_live_buffer_overlap}; "
        f"with a lifetime-overlapping texture: {targets_with_live_texture_overlap}; "
        f"named by an explicit alias barrier: {target_barrier_count}.",
        "",
        "First targets, ordered by explicit barriers and live overlap:",
        "",
    ])
    output.extend(markdown_table(
        ["cookie", "shape", "heap", "offset", "size", "all overlaps", "live buffers", "live textures", "alias barriers"],
        target_rows[:100],
    ))
    output.extend(["", "## Explicit alias barriers", ""])
    output.extend(markdown_table(
        ["before kind", "after kind", "barriers"],
        ((before, after, count) for (before, after), count in barrier_kinds.most_common()),
    ))

    if missing_target_creates:
        output.extend([
            "",
            "Missing target cookies (the alias create cap or an invalid gate can cause this):",
            "",
            ", ".join(str(cookie) for cookie in sorted(missing_target_creates)[:200]),
        ])

    output.extend([
        "",
        "## Interpretation gate",
        "",
        "A target with no live range overlap and no explicit alias barrier is evidence against "
        "placed-resource aliasing as its population path. A live overlap or barrier selects a "
        "narrower descriptor/resource-use trace, but still does not justify a behavior-changing "
        "flag without demonstrating that the target reaches a shader and changes rendering.",
    ])

    args.output.write_text("\n".join(output) + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
