#!/usr/bin/env python3
"""Summarize bounded IL2TEX telemetry without copying the full Proton log."""

from __future__ import annotations

import argparse
import collections
import pathlib
import re
from typing import Any, Iterable


EVENT_RE = re.compile(
    r"IL2TEX (create|srv|copy_resource|copy|destroy|suppressed)\s+(.*)$"
)
FIELD_RE = re.compile(r"([a-z_]+)=([^\s]+)")
FORMAT_NAMES = {
    28: "R8G8B8A8_UNORM",
    29: "R8G8B8A8_UNORM_SRGB",
    71: "BC1_UNORM",
    72: "BC1_UNORM_SRGB",
    77: "BC3_UNORM",
    78: "BC3_UNORM_SRGB",
    87: "B8G8R8A8_UNORM",
    88: "B8G8R8X8_UNORM",
    91: "B8G8R8A8_UNORM_SRGB",
    98: "BC7_UNORM",
    99: "BC7_UNORM_SRGB",
}
BLOCK_COMPRESSED_COLOR_FORMATS = {71, 72, 74, 75, 77, 78, 80, 81, 83, 84, 95, 96, 98, 99}
RESOURCE_DIMENSION_NAMES = {
    0: "UNKNOWN",
    1: "BUFFER",
    2: "TEXTURE1D",
    3: "TEXTURE2D",
    4: "TEXTURE3D",
}


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


def parse_events(lines: Iterable[str]) -> list[tuple[int, str, dict[str, Any]]]:
    events: list[tuple[int, str, dict[str, Any]]] = []
    for line_number, line in enumerate(lines, 1):
        match = EVENT_RE.search(line)
        if not match:
            continue
        fields = {
            name: parse_value(value)
            for name, value in FIELD_RE.findall(match.group(2))
        }
        events.append((line_number, match.group(1), fields))
    return events


def markdown_table(headers: list[str], rows: Iterable[Iterable[Any]]) -> list[str]:
    rendered = ["| " + " | ".join(headers) + " |"]
    rendered.append("|" + "|".join("---" for _ in headers) + "|")
    for row in rows:
        rendered.append("| " + " | ".join(str(value) for value in row) + " |")
    return rendered


def format_shape(fields: dict[str, Any]) -> str:
    return (
        f"{dimension_name(fields.get('dimension', '?'))} "
        f"{fields.get('width', '?')}x{fields.get('height', '?')}x"
        f"{fields.get('depth_or_layers', '?')}; mips={fields.get('mips', '?')}; "
        f"fmt={format_name(fields.get('format', '?'))}"
    )


def format_name(value: Any) -> str:
    if isinstance(value, int):
        return FORMAT_NAMES.get(value, f"DXGI#{value}")
    return str(value)


def dimension_name(value: Any) -> str:
    if isinstance(value, int):
        return RESOURCE_DIMENSION_NAMES.get(value, f"DIMENSION#{value}")
    return str(value)


def parse_vec3(value: Any, separator: str) -> tuple[int, int, int] | None:
    if not isinstance(value, str):
        return None
    parts = value.split(separator)
    if len(parts) != 3:
        return None
    try:
        return int(parts[0]), int(parts[1]), int(parts[2])
    except ValueError:
        return None


def expected_subresource_count(created: dict[str, Any]) -> int:
    # D3D12_RESOURCE_DIMENSION_TEXTURE3D has one subresource per mip. For 1D/2D
    # textures, array layers each carry their own mip chain.
    if created.get("dimension") == 4:
        return int(created["mips"])
    return int(created["mips"]) * int(created["depth_or_layers"])


def upload_geometrically_covers_resource(
    created: dict[str, Any],
    regions: dict[int, list[tuple[tuple[int, int, int], tuple[int, int, int]]]],
) -> bool:
    mips = int(created["mips"])
    expected_count = expected_subresource_count(created)
    dimension = int(created["dimension"])

    if any(subresource not in regions for subresource in range(expected_count)):
        return False

    for subresource in range(expected_count):
        mip = subresource if dimension == 4 else subresource % mips
        expected_width = max(1, int(created["width"]) >> mip)
        expected_height = max(1, int(created["height"]) >> mip)
        expected_depth = max(1, int(created["depth_or_layers"]) >> mip) if dimension == 4 else 1
        z_ranges: list[tuple[int, int]] = []

        for offset, extent in regions[subresource]:
            if offset[0] != 0 or offset[1] != 0:
                continue
            if extent[0] < expected_width or extent[1] < expected_height:
                continue
            z_ranges.append((offset[2], offset[2] + extent[2]))

        if not z_ranges:
            return False

        covered_until = 0
        for begin, end in sorted(z_ranges):
            if begin > covered_until:
                break
            covered_until = max(covered_until, end)
        if covered_until < expected_depth:
            return False

    return True


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("log", type=pathlib.Path)
    parser.add_argument("--output", type=pathlib.Path, required=True)
    args = parser.parse_args()

    with args.log.open("r", encoding="utf-8", errors="replace") as stream:
        events = parse_events(stream)

    counts = collections.Counter(event for _, event, _ in events)
    creates: dict[int, dict[str, Any]] = {}
    activity: dict[int, collections.Counter[str]] = collections.defaultdict(collections.Counter)
    destroyed_at: dict[int, int] = {}
    after_destroy: list[tuple[int, str, int]] = []
    shape_counts: collections.Counter[tuple[Any, ...]] = collections.Counter()
    srv_counts: collections.Counter[tuple[Any, ...]] = collections.Counter()
    copy_counts: collections.Counter[tuple[Any, ...]] = collections.Counter()
    upload_shape_counts: collections.Counter[tuple[Any, ...]] = collections.Counter()
    uploaded_subresources: dict[int, set[int]] = collections.defaultdict(set)
    upload_regions: dict[
        int, dict[int, list[tuple[tuple[int, int, int], tuple[int, int, int]]]]
    ] = collections.defaultdict(lambda: collections.defaultdict(list))
    full_uploads: dict[int, bool] = collections.defaultdict(lambda: True)

    for line_number, event, fields in events:
        if event == "create":
            cookie = int(fields["cookie"])
            creates[cookie] = fields
            shape_counts[
                (
                    fields.get("allocation"),
                    fields.get("dimension"),
                    fields.get("width"),
                    fields.get("height"),
                    fields.get("depth_or_layers"),
                    fields.get("mips"),
                    fields.get("format"),
                    fields.get("resource_flags"),
                )
            ] += 1
            activity[cookie]["create"] += 1
        elif event == "srv":
            cookie = int(fields["cookie"])
            activity[cookie]["srv"] += 1
            srv_counts[
                (
                    fields.get("first_mip"),
                    fields.get("mip_count"),
                    fields.get("min_lod"),
                    fields.get("first_layer"),
                    fields.get("layer_count"),
                    fields.get("view_dimension"),
                )
            ] += 1
            if cookie in destroyed_at:
                after_destroy.append((line_number, event, cookie))
        elif event in ("copy", "copy_resource"):
            src_cookie = int(fields["src_cookie"])
            dst_cookie = int(fields["dst_cookie"])
            activity[src_cookie]["copy_src"] += 1
            activity[dst_cookie]["copy_dst"] += 1
            copy_counts[
                (
                    event,
                    fields.get("batch", "whole"),
                    fields.get("list_type"),
                    fields.get("extent", fields.get("dst_extent", "whole")),
                    fields.get("image_mip", fields.get("dst_mip", "all")),
                    fields.get("full_subresource", "n/a"),
                )
            ] += 1
            for cookie in (src_cookie, dst_cookie):
                if cookie in destroyed_at:
                    after_destroy.append((line_number, event, cookie))
            if event == "copy" and fields.get("batch") == 1:
                if isinstance(fields.get("dst_subresource"), int):
                    uploaded_subresources[dst_cookie].add(fields["dst_subresource"])
                    offset = parse_vec3(fields.get("image_offset"), ",")
                    extent = parse_vec3(fields.get("extent"), "x")
                    if offset is not None and extent is not None:
                        upload_regions[dst_cookie][fields["dst_subresource"]].append((offset, extent))
                if fields.get("full_subresource") != 1:
                    full_uploads[dst_cookie] = False
        elif event == "destroy":
            cookie = int(fields["cookie"])
            activity[cookie]["destroy"] += 1
            destroyed_at[cookie] = line_number

    active_rows = []
    for cookie, cookie_activity in activity.items():
        score = cookie_activity["srv"] + cookie_activity["copy_src"] + cookie_activity["copy_dst"]
        if not score:
            continue
        active_rows.append(
            (
                score,
                cookie,
                format_shape(creates.get(cookie, {})),
                cookie_activity["srv"],
                cookie_activity["copy_dst"],
                cookie_activity["copy_src"],
                cookie_activity["destroy"],
            )
        )
    active_rows.sort(reverse=True)

    for cookie, subresources in uploaded_subresources.items():
        created = creates.get(cookie)
        if not created:
            continue
        upload_shape_counts[
            (
                created.get("dimension"),
                created.get("width"),
                created.get("height"),
                created.get("depth_or_layers"),
                created.get("mips"),
                created.get("format"),
            )
        ] += len(subresources)

    compressed_completeness = collections.Counter()
    partial_compressed_rows = []
    no_upload_shape_counts: collections.Counter[tuple[Any, ...]] = collections.Counter()
    no_upload_activity_counts = collections.Counter()
    no_upload_activity_rows = []
    for cookie, created in creates.items():
        if created.get("format") not in BLOCK_COMPRESSED_COLOR_FORMATS or created.get("mips", 1) <= 1:
            continue
        expected = expected_subresource_count(created)
        actual = len(uploaded_subresources.get(cookie, set()))
        if upload_geometrically_covers_resource(created, upload_regions.get(cookie, {})):
            classification = "complete"
        elif actual == 0:
            classification = "no buffer upload logged"
        else:
            classification = "partial"
        compressed_completeness[classification] += 1
        if classification == "partial":
            partial_compressed_rows.append(
                (
                    cookie,
                    format_shape(created),
                    expected,
                    actual,
                    "yes" if full_uploads[cookie] else "no",
                )
            )
        elif classification == "no buffer upload logged":
            no_upload_shape_counts[
                (
                    created.get("dimension"),
                    created.get("width"),
                    created.get("height"),
                    created.get("depth_or_layers"),
                    created.get("mips"),
                    created.get("format"),
                )
            ] += 1
            cookie_activity = activity[cookie]
            if cookie_activity["copy_dst"]:
                activity_class = "incoming texture copy"
            elif cookie_activity["srv"]:
                activity_class = "SRV with no logged incoming copy"
            else:
                activity_class = "no SRV or incoming copy logged"
            no_upload_activity_counts[activity_class] += 1
            no_upload_activity_rows.append(
                (
                    cookie,
                    format_shape(created),
                    cookie_activity["srv"],
                    cookie_activity["copy_dst"],
                    cookie_activity["copy_src"],
                    created.get("heap_offset"),
                )
            )

    output: list[str] = [
        "# Ordinary texture trace analysis",
        "",
        "This report is generated from bounded, read-only `IL2TEX` telemetry. "
        "It identifies resource classes for narrower experiments; it does not by itself prove a defect.",
        "",
        "## Event counts",
        "",
    ]
    output.extend(
        markdown_table(
            ["event", "count"],
            ((name, counts.get(name, 0)) for name in
             ("create", "srv", "copy", "copy_resource", "destroy", "suppressed")),
        )
    )

    output.extend(["", "## Most common created texture shapes", ""])
    output.extend(
        markdown_table(
            ["count", "allocation", "dimension", "width", "height", "depth/layers", "mips", "format", "VKD3D internal flags"],
            ((count, shape[0], dimension_name(shape[1]), *shape[2:6], format_name(shape[6]), shape[7])
             for shape, count in shape_counts.most_common(25)),
        )
    )

    output.extend(["", "## Most common normalized SRV selections", ""])
    nonzero_min_lod = sum(
        count for selection, count in srv_counts.items()
        if selection[2] not in (0, 0.0, None)
    )
    output.append(
        f"Logged SRV descriptions: {counts.get('srv', 0)}; "
        f"non-zero `ResourceMinLODClamp`: {nonzero_min_lod}."
    )
    output.append("")
    output.extend(
        markdown_table(
            ["count", "first mip", "mip count", "min LOD", "first layer", "layers", "view dimension"],
            ((count, *selection) for selection, count in srv_counts.most_common(25)),
        )
    )

    output.extend(["", "## Most common copy classes", ""])
    output.extend(
        markdown_table(
            ["count", "operation", "batch", "list type", "extent", "destination mip", "full subresource"],
            ((count, *copy_class) for copy_class, count in copy_counts.most_common(25)),
        )
    )

    output.extend(["", "## Buffer-to-image upload destinations", ""])
    output.extend(
        markdown_table(
            ["uploaded subresources", "dimension", "width", "height", "depth/layers", "mips", "format"],
            ((count, dimension_name(shape[0]), *shape[1:5], format_name(shape[5]))
             for shape, count in upload_shape_counts.most_common(25)),
        )
    )

    output.extend(["", "## Block-compressed mip-chain completeness", ""])
    output.append(
        "This compares unique logged buffer-to-image destination subresources with "
        "the D3D12 subresource count and verifies geometric coverage. For 3D textures, "
        "separate Z-slice copies are combined per mip. The copy-event cap can make resources created "
        "near or after suppression appear partial; a complete result is positive evidence "
        "that every expected subresource was fully written."
    )
    output.extend(["", *markdown_table(
        ["classification", "resources"], compressed_completeness.most_common()
    )])
    if partial_compressed_rows:
        output.extend(["", "Partial resources:", "", *markdown_table(
            ["cookie", "created shape", "expected", "logged", "all logged copies full"],
            partial_compressed_rows,
        )])
    if no_upload_shape_counts:
        output.extend(["", "Most common shapes with no logged buffer upload:", "", *markdown_table(
            ["resources", "dimension", "width", "height", "depth/layers", "mips", "format"],
            ((count, dimension_name(shape[0]), *shape[1:5], format_name(shape[5]))
             for shape, count in no_upload_shape_counts.most_common(15)),
        )])
        output.extend(["", "Activity of resources with no logged buffer upload:", "", *markdown_table(
            ["classification", "resources"], no_upload_activity_counts.most_common()
        )])
        output.extend(["", "First resources with no logged buffer upload:", "", *markdown_table(
            ["cookie", "created shape", "SRVs", "copies into", "copies out", "heap offset"],
            sorted(no_upload_activity_rows, key=lambda row: (row[3], row[2]), reverse=True)[:50],
        )])

    output.extend(["", "## Most active resources", ""])
    output.extend(
        markdown_table(
            ["activity", "cookie", "created shape", "SRVs", "copies into", "copies out", "destroyed"],
            active_rows[:40],
        )
    )

    output.extend(["", "## Lifetime ordering diagnostic", ""])
    if after_destroy:
        output.append(
            f"Observed {len(after_destroy)} logged uses after a destroy marker. "
            "Threaded log ordering must be ruled out before treating these as use-after-free evidence."
        )
        output.extend(["", *markdown_table(
            ["log line", "event", "cookie"], after_destroy[:100]
        )])
    else:
        output.append("No logged SRV or copy event followed a destroy marker for the same cookie.")

    args.output.write_text("\n".join(output) + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
