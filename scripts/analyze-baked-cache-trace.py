#!/usr/bin/env python3
"""Summarize bounded IL2CACHE producer-to-SRV telemetry."""

from __future__ import annotations

import argparse
import collections
import gzip
import re
from dataclasses import dataclass, field
from pathlib import Path


EVENT_RE = re.compile(r"IL2CACHE (enabled:|create |srv |copy_image |copy_buffer_image |copy_resource |barrier |enhanced_barrier |destroy |suppressed )")
FIELD_RE = re.compile(r"([a-z_]+)=([^\s.]+)")


@dataclass
class Resource:
    cookie: int
    format: str = "unknown"
    allocation: str = "unknown"
    flags: str = "unknown"
    created_at: int | None = None
    destroyed_at: int | None = None
    srv_lines: list[int] = field(default_factory=list)
    incoming: list[tuple[int, dict[str, str]]] = field(default_factory=list)
    outgoing: list[tuple[int, dict[str, str]]] = field(default_factory=list)
    barriers: list[tuple[int, dict[str, str]]] = field(default_factory=list)


def read_lines(path: Path):
    opener = gzip.open if path.suffix == ".gz" else open
    with opener(path, "rt", encoding="utf-8", errors="replace") as stream:
        yield from stream


def fields(line: str) -> dict[str, str]:
    return dict(FIELD_RE.findall(line))


def as_int(value: str | None) -> int | None:
    if value is None:
        return None
    try:
        return int(value, 0)
    except ValueError:
        return None


def resource_for(resources: dict[int, Resource], cookie: int) -> Resource:
    return resources.setdefault(cookie, Resource(cookie=cookie))


def parse_triplet(value: str | None) -> tuple[int, int, int] | None:
    if value is None:
        return None
    try:
        first, second, third = value.split("x" if "x" in value else ",")
        return int(first), int(second), int(third)
    except (ValueError, TypeError):
        return None


def union_area(rectangles: list[tuple[int, int, int, int]]) -> int:
    """Return the exact union area of axis-aligned integer rectangles."""
    xs = sorted({x for left, _, right, _ in rectangles for x in (left, right)})
    area = 0
    for left, right in zip(xs, xs[1:]):
        intervals = sorted(
            (top, bottom) for rect_left, top, rect_right, bottom in rectangles
            if rect_left <= left and rect_right >= right and bottom > top
        )
        covered = 0
        if intervals:
            current_top, current_bottom = intervals[0]
            for top, bottom in intervals[1:]:
                if top > current_bottom:
                    covered += current_bottom - current_top
                    current_top, current_bottom = top, bottom
                else:
                    current_bottom = max(current_bottom, bottom)
            covered += current_bottom - current_top
        area += (right - left) * covered
    return area


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("log", type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()

    resources: dict[int, Resource] = {}
    marker_count = 0
    suppressed_count = 0
    copy_count = 0
    barrier_count = 0

    for line_number, line in enumerate(read_lines(args.log), 1):
        match = EVENT_RE.search(line)
        if not match:
            continue
        event = match.group(1).strip()
        values = fields(line)

        if event == "enabled:":
            marker_count += 1
            continue
        if event == "suppressed":
            suppressed_count += 1
            continue

        cookie = as_int(values.get("cookie"))
        if event == "create" and cookie is not None:
            resource = resource_for(resources, cookie)
            resource.format = values.get("format", "unknown")
            resource.allocation = values.get("allocation", "unknown")
            resource.flags = values.get("resource_flags", "unknown")
            resource.created_at = line_number
        elif event == "srv" and cookie is not None:
            resource_for(resources, cookie).srv_lines.append(line_number)
        elif event in {"barrier", "enhanced_barrier"} and cookie is not None:
            resource_for(resources, cookie).barriers.append((line_number, values))
            barrier_count += 1
        elif event == "destroy" and cookie is not None:
            resource_for(resources, cookie).destroyed_at = line_number
        elif event.startswith("copy_"):
            copy_count += 1
            src_cookie = as_int(values.get("src_cookie"))
            dst_cookie = as_int(values.get("dst_cookie"))
            if src_cookie is not None:
                resource_for(resources, src_cookie).outgoing.append((line_number, values))
            if dst_cookie is not None:
                resource_for(resources, dst_cookie).incoming.append((line_number, values))

    created = [resource for resource in resources.values() if resource.created_at is not None]
    bc3 = [resource for resource in created if resource.format.lower() == "0x4d"]
    bc3_no_incoming = [resource for resource in bc3 if not resource.incoming]
    bc3_no_srv = [resource for resource in bc3 if not resource.srv_lines]
    bc3_srv_before_copy = [
        resource for resource in bc3
        if resource.srv_lines and resource.incoming and min(resource.srv_lines) < min(item[0] for item in resource.incoming)
    ]
    bc3_srv_without_copy = [resource for resource in bc3 if resource.srv_lines and not resource.incoming]
    full_writes = sum(
        1 for resource in bc3 for _, values in resource.incoming
        if values.get("full_resource") == "1" or values.get("full_subresource") == "1"
    )

    bc3_copy_shapes: collections.Counter[tuple[int, int, int]] = collections.Counter()
    coverage: list[tuple[int, int, int, int]] = []
    for resource in bc3:
        current_rectangles: list[tuple[int, int, int, int]] = []
        scaled_rectangles: list[tuple[int, int, int, int]] = []
        for _, values in resource.incoming:
            extent = parse_triplet(values.get("extent"))
            offset = parse_triplet(values.get("image_offset"))
            if extent is None or offset is None or extent[2] != 1 or offset[2] != 0:
                continue
            width, height, _ = extent
            x, y, _ = offset
            bc3_copy_shapes[extent] += 1
            current_rectangles.append((x, y, min(x + width, 2048), min(y + height, 2048)))
            scaled_rectangles.append((x, y, min(x + 4 * width, 2048), min(y + 4 * height, 2048)))
        if current_rectangles:
            coverage.append((resource.cookie, len(current_rectangles),
                    union_area(current_rectangles), union_area(scaled_rectangles)))

    format_counts = collections.Counter(resource.format for resource in created)
    signatures = collections.Counter(
        (resource.format, len(resource.incoming), len(resource.outgoing),
         len(resource.srv_lines), len(resource.barriers), resource.destroyed_at is not None)
        for resource in created
    )

    output: list[str] = [
        "# D06 baked-cache trace analysis",
        "",
        f"- Instrumentation markers: {marker_count}",
        f"- Matching resource creates: {len(created)}",
        f"- Copy events involving the class: {copy_count}",
        f"- Barrier events involving the class: {barrier_count}",
        f"- Suppression markers: {suppressed_count}",
        f"- BC3 (`0x4d`) cache resources: {len(bc3)}",
        f"- BC3 resources with no incoming logged copy: {len(bc3_no_incoming)}",
        f"- BC3 resources with no SRV creation: {len(bc3_no_srv)}",
        f"- BC3 resources first exposed as SRV before their first incoming copy: {len(bc3_srv_before_copy)}",
        f"- BC3 resources with an SRV but no incoming copy: {len(bc3_srv_without_copy)}",
        f"- Incoming BC3 copies marked full-resource/full-subresource: {full_writes}",
        "",
        "## Formats",
        "",
        "| DXGI format | Resources |",
        "|---|---:|",
    ]
    output.extend(f"| `{fmt}` | {count} |" for fmt, count in sorted(format_counts.items()))

    output.extend([
        "",
        "## Buffer-to-BC3 copy geometry",
        "",
        "| Emitted Vulkan extent | Count |",
        "|---|---:|",
    ])
    output.extend(
        f"| {width}x{height}x{depth} | {count} |"
        for (width, height, depth), count in sorted(bc3_copy_shapes.items())
    )
    output.extend([
        "",
        "The square interiors are placed on a 256-texel grid although their emitted extent is 64x64. "
        "The projected column below keeps each destination offset fixed and expands each observed source "
        "element to a 4x4 BC3 block. D06 did not log the placed-footprint DXGI format, so this is a "
        "geometry projection rather than proof that every interior uses `R32G32B32A32_UINT`.",
        "",
        "| BC3 cookie | Logged copies | Current union coverage | Projected 4x union coverage |",
        "|---:|---:|---:|---:|",
    ])
    total_pixels = 2048 * 2048
    output.extend(
        f"| {cookie} | {count} | {current / total_pixels:.2%} | {scaled / total_pixels:.2%} |"
        for cookie, count, current, scaled in sorted(coverage)
    )

    output.extend([
        "",
        "## Lifecycle signatures",
        "",
        "| Format | Incoming copies | Outgoing copies | SRVs | Barriers | Destroyed | Resources |",
        "|---|---:|---:|---:|---:|---|---:|",
    ])
    for signature, count in sorted(signatures.items()):
        fmt, incoming, outgoing, srvs, barriers, destroyed = signature
        output.append(f"| `{fmt}` | {incoming} | {outgoing} | {srvs} | {barriers} | {'yes' if destroyed else 'no'} | {count} |")

    suspicious = sorted(
        set(resource.cookie for resource in bc3_srv_without_copy + bc3_srv_before_copy)
    )
    output.extend([
        "",
        "## Ordering candidates",
        "",
        "BC3 cookies with an SRV but no incoming copy, or an SRV observed before",
        "their first incoming copy:",
        "",
        ", ".join(f"`{cookie}`" for cookie in suspicious) if suspicious else "None.",
        "",
        "This trace proves API ordering only. SRV creation does not prove that a draw",
        "sampled the descriptor, and unlogged render/UAV writes would require a later",
        "producer-specific extension of the diagnostic.",
        "",
    ])

    rendered = "\n".join(output)
    if args.output:
        args.output.write_text(rendered, encoding="utf-8")
    else:
        print(rendered, end="")
    return 0 if marker_count == 1 else 1


if __name__ == "__main__":
    raise SystemExit(main())
