#!/usr/bin/env python3
"""Validate the bounded D05 BC3 border-copy diagnostic log."""

from __future__ import annotations

import argparse
import collections
import pathlib
import re
import sys


ADJUSTMENT_RE = re.compile(
    r"IL2BCCOPY adjust seq=(?P<sequence>\d+) list_type=(?P<list_type>\d+) "
    r"dst_cookie=(?P<cookie>\d+) original_extent=(?P<ow>\d+)x(?P<oh>\d+)x(?P<od>\d+) "
    r"emitted_extent=(?P<ew>\d+)x(?P<eh>\d+)x(?P<ed>\d+) "
    r"image_offset=(?P<ix>-?\d+),(?P<iy>-?\d+),(?P<iz>-?\d+) "
    r"src_box=(?P<sl>\d+),(?P<st>\d+),(?P<sf>\d+)-"
    r"(?P<sr>\d+),(?P<sb>\d+),(?P<sk>\d+) "
    r"footprint=(?P<fw>\d+)x(?P<fh>\d+)x(?P<fd>\d+) "
    r"row_pitch=(?P<row_pitch>\d+) buffer_offset=(?P<buffer_offset>\d+)\."
)

EXPECTED_TRANSFORMS = {
    (1, 64, 1): (4, 64, 1),
    (1, 128, 1): (4, 128, 1),
    (64, 1, 1): (64, 4, 1),
    (128, 1, 1): (128, 4, 1),
}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("log", type=pathlib.Path)
    parser.add_argument("--output", type=pathlib.Path)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    text = args.log.read_text(encoding="utf-8", errors="replace")
    enabled_count = text.count("IL2BCCOPY enabled ")
    limit_count = text.count("IL2BCCOPY adjustment log limit")
    records = [{key: int(value) for key, value in match.groupdict().items()}
               for match in ADJUSTMENT_RE.finditer(text)]

    errors: list[str] = []
    warnings: list[str] = []
    shapes: collections.Counter[tuple[int, int, int]] = collections.Counter()
    emitted_shapes: collections.Counter[tuple[int, int, int]] = collections.Counter()
    footprints: collections.Counter[tuple[int, int, int, int]] = collections.Counter()
    cookies: set[int] = set()

    if enabled_count != 1:
        errors.append(f"expected one enable marker, found {enabled_count}")
    if not records:
        errors.append("the diagnostic was enabled but no matching border copy was adjusted")
    if limit_count:
        warnings.append("the 1,024-line adjustment cap was reached; later matching copies were still normalized")

    sequences = [record["sequence"] for record in records]
    if sequences and sequences != list(range(1, len(sequences) + 1)):
        errors.append("adjustment sequence numbers are missing, duplicated, or out of order")

    for record in records:
        original = (record["ow"], record["oh"], record["od"])
        emitted = (record["ew"], record["eh"], record["ed"])
        shapes[original] += 1
        emitted_shapes[emitted] += 1
        footprints[(record["fw"], record["fh"], record["fd"], record["row_pitch"])] += 1
        cookies.add(record["cookie"])

        if original not in EXPECTED_TRANSFORMS:
            errors.append(f"sequence {record['sequence']} has unexpected original extent {original}")
        elif emitted != EXPECTED_TRANSFORMS[original]:
            errors.append(
                f"sequence {record['sequence']} emitted {emitted}, expected {EXPECTED_TRANSFORMS[original]}"
            )
        if record["ix"] < 0 or record["iy"] < 0 or record["iz"] != 0:
            errors.append(f"sequence {record['sequence']} has an invalid destination offset")
        if record["ix"] % 4 or record["iy"] % 4:
            errors.append(f"sequence {record['sequence']} has a non-block-aligned destination offset")
        if record["sl"] % 4 or record["st"] % 4 or record["sf"] != 0:
            errors.append(f"sequence {record['sequence']} has a non-block-aligned source offset")
        source_extent = (
            record["sr"] - record["sl"],
            record["sb"] - record["st"],
            record["sk"] - record["sf"],
        )
        if source_extent != original:
            errors.append(f"sequence {record['sequence']} source box {source_extent} differs from {original}")
        if record["ix"] + record["ew"] > 2048 or record["iy"] + record["eh"] > 2048:
            errors.append(f"sequence {record['sequence']} exceeds the destination mip")

        # BC3 needs 16 bytes for each physical 4x4 block. A footprint whose
        # virtual thin dimension is one texel still owns that complete block.
        physical_blocks_per_row = (record["fw"] + 3) // 4
        if record["fd"] != 1 or record["row_pitch"] < physical_blocks_per_row * 16:
            errors.append(f"sequence {record['sequence']} source footprint lacks a complete BC3 row")

    if records and len(records) != 432:
        warnings.append(
            f"D05 logged {len(records)} adjustments; D02 logged 432, but scene duration and cache demand can change the count"
        )

    status = "valid" if not errors else "invalid"
    lines = [
        "# D05 BC3 border-copy analysis",
        "",
        f"- Instrumentation validity: **{status}**",
        f"- Enable markers: {enabled_count}",
        f"- Logged adjustments: {len(records)}",
        f"- Destination resources: {len(cookies)}",
        f"- Adjustment-log cap markers: {limit_count}",
        "",
        "## Original to emitted extents",
        "",
        "| Original | Emitted | Count |",
        "|---|---|---:|",
    ]
    for original, count in sorted(shapes.items()):
        emitted = EXPECTED_TRANSFORMS.get(original)
        emitted_text = "unknown" if emitted is None else f"{emitted[0]}x{emitted[1]}x{emitted[2]}"
        lines.append(f"| {original[0]}x{original[1]}x{original[2]} | {emitted_text} | {count} |")

    lines.extend(["", "## Source footprints", "", "| Width x height x depth | Row pitch | Count |", "|---|---:|---:|"])
    for (width, height, depth, row_pitch), count in sorted(footprints.items()):
        lines.append(f"| {width}x{height}x{depth} | {row_pitch} | {count} |")

    if warnings:
        lines.extend(["", "## Warnings", ""])
        lines.extend(f"- {warning}" for warning in warnings)
    if errors:
        lines.extend(["", "## Errors", ""])
        lines.extend(f"- {error}" for error in errors[:100])
        if len(errors) > 100:
            lines.append(f"- {len(errors) - 100} additional errors suppressed")

    lines.extend([
        "",
        "## Interpretation",
        "",
        "A valid result proves that D05 recognized only the observed Korea terrain-cache border class, "
        "that every source row contained at least one complete physical BC3 block, and that the emitted "
        "Vulkan extents were block-complete and remained inside the 2048x2048 mip. Visual classification "
        "is still required to decide whether this compatibility behavior affects seams, missing pages, both, or neither.",
        "",
    ])
    output = "\n".join(lines)
    if args.output:
        args.output.write_text(output, encoding="utf-8")
    else:
        print(output)
    return 0 if not errors else 1


if __name__ == "__main__":
    sys.exit(main())
