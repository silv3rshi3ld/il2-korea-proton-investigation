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
        f"{fields.get('width', '?')}x{fields.get('height', '?')}x"
        f"{fields.get('depth_or_layers', '?')}; mips={fields.get('mips', '?')}; "
        f"fmt={fields.get('format', '?')}"
    )


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

    for line_number, event, fields in events:
        if event == "create":
            cookie = int(fields["cookie"])
            creates[cookie] = fields
            shape_counts[
                (
                    fields.get("allocation"),
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
            ["count", "allocation", "width", "height", "depth/layers", "mips", "format", "flags"],
            ((count, *shape) for shape, count in shape_counts.most_common(25)),
        )
    )

    output.extend(["", "## Most common normalized SRV selections", ""])
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
