#!/usr/bin/env python3
"""Validate the bounded D05c RGBA32_UINT-to-BC3 reinterpret-copy diagnostic."""

from __future__ import annotations

import argparse
import collections
import pathlib
import re
import sys


CANDIDATE_RE = re.compile(
    r"IL2BCCOPY candidate seq=(?P<sequence>\d+) list_type=(?P<list_type>\d+) "
    r"dst_cookie=(?P<cookie>\d+) src_box_present=(?P<src_box_present>\d+) "
    r"src_format=(?P<src_format>0x[0-9a-f]+) dst_format=(?P<dst_format>0x[0-9a-f]+) "
    r"original_extent=(?P<ow>\d+)x(?P<oh>\d+)x(?P<od>\d+) "
    r"image_offset=(?P<ix>-?\d+),(?P<iy>-?\d+),(?P<iz>-?\d+) "
    r"source_origin=(?P<sl>\d+),(?P<st>\d+),(?P<sf>\d+) "
    r"source_extent=(?P<sw>\d+)x(?P<sh>\d+)x(?P<sd>\d+) "
    r"footprint=(?P<fw>\d+)x(?P<fh>\d+)x(?P<fd>\d+) "
    r"row_pitch=(?P<row_pitch>\d+) buffer_row_length=(?P<buffer_row_length>\d+) "
    r"buffer_image_height=(?P<buffer_image_height>\d+) buffer_offset=(?P<buffer_offset>\d+)\."
)

ADJUSTMENT_RE = re.compile(
    r"IL2BCCOPY adjust seq=(?P<sequence>\d+) candidate_seq=(?P<candidate_sequence>\d+) "
    r"list_type=(?P<list_type>\d+) dst_cookie=(?P<cookie>\d+) "
    r"src_box_present=(?P<src_box_present>\d+) src_format=(?P<src_format>0x[0-9a-f]+) "
    r"dst_format=(?P<dst_format>0x[0-9a-f]+) "
    r"original_extent=(?P<ow>\d+)x(?P<oh>\d+)x(?P<od>\d+) "
    r"emitted_extent=(?P<ew>\d+)x(?P<eh>\d+)x(?P<ed>\d+) "
    r"image_offset=(?P<ix>-?\d+),(?P<iy>-?\d+),(?P<iz>-?\d+) "
    r"source_origin=(?P<sl>\d+),(?P<st>\d+),(?P<sf>\d+) "
    r"source_extent=(?P<sw>\d+)x(?P<sh>\d+)x(?P<sd>\d+) "
    r"footprint=(?P<fw>\d+)x(?P<fh>\d+)x(?P<fd>\d+) "
    r"row_pitch=(?P<row_pitch>\d+) buffer_row_length=(?P<buffer_row_length>\d+) "
    r"buffer_image_height=(?P<buffer_image_height>\d+) buffer_offset=(?P<buffer_offset>\d+)\."
)

REJECTION_RE = re.compile(
    r"IL2BCCOPY reject candidate_seq=(?P<candidate_sequence>\d+) mask=(?P<mask>0x[0-9a-f]+)\."
)

EXPECTED_TRANSFORMS = {
    (1, 64, 1): (4, 256, 1),
    (1, 128, 1): (4, 512, 1),
    (64, 1, 1): (256, 4, 1),
    (128, 1, 1): (512, 4, 1),
}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("log", type=pathlib.Path)
    parser.add_argument("--output", type=pathlib.Path)
    return parser.parse_args()


def parse_records(pattern: re.Pattern[str], text: str) -> list[dict[str, int]]:
    records = []
    for match in pattern.finditer(text):
        records.append({key: int(value, 0) for key, value in match.groupdict().items()})
    return records


def contiguous(records: list[dict[str, int]], field: str = "sequence") -> bool:
    values = [record[field] for record in records]
    return not values or values == list(range(1, len(values) + 1))


def main() -> int:
    args = parse_args()
    text = args.log.read_text(encoding="utf-8", errors="replace")
    enabled_count = text.count("IL2BCCOPY enabled ")
    limit_count = text.count("IL2BCCOPY adjustment log limit")
    candidates = parse_records(CANDIDATE_RE, text)
    adjustments = parse_records(ADJUSTMENT_RE, text)
    rejections = parse_records(REJECTION_RE, text)

    errors: list[str] = []
    warnings: list[str] = []
    shapes: collections.Counter[tuple[int, int, int]] = collections.Counter()
    footprints: collections.Counter[tuple[int, int, int, int]] = collections.Counter()
    source_forms: collections.Counter[str] = collections.Counter()
    source_formats: collections.Counter[int] = collections.Counter()
    rejection_masks: collections.Counter[int] = collections.Counter()
    cookies: set[int] = set()

    if enabled_count != 1:
        errors.append(f"expected one enable marker, found {enabled_count}")
    if not candidates:
        errors.append("the diagnostic was enabled but no target-class candidate was recorded")
    if not adjustments:
        errors.append("no candidate passed the physical-block safety checks and was adjusted")
    if not contiguous(candidates):
        errors.append("candidate sequence numbers are missing, duplicated, or out of order")
    if not contiguous(adjustments):
        errors.append("adjustment sequence numbers are missing, duplicated, or out of order")
    if limit_count:
        warnings.append("the 1,024-line adjustment cap was reached; later matching copies were still normalized")

    candidate_ids = {record["sequence"] for record in candidates}
    adjusted_ids = {record["candidate_sequence"] for record in adjustments}
    rejected_ids = {record["candidate_sequence"] for record in rejections}
    for record in rejections:
        rejection_masks[record["mask"]] += 1
    if adjusted_ids & rejected_ids:
        errors.append("one or more candidates were both adjusted and rejected")
    if (adjusted_ids | rejected_ids) - candidate_ids:
        errors.append("an adjustment or rejection refers to an unlogged candidate")
    if candidates and len(candidates) < 1024 and candidate_ids != adjusted_ids | rejected_ids:
        errors.append("one or more logged candidates have neither an adjustment nor rejection record")
    if rejections:
        errors.append(f"{len(rejections)} target-class candidates failed the safety checks")

    for record in adjustments:
        original = (record["ow"], record["oh"], record["od"])
        emitted = (record["ew"], record["eh"], record["ed"])
        shapes[original] += 1
        footprints[(record["fw"], record["fh"], record["fd"], record["row_pitch"])] += 1
        source_forms["explicit source box" if record["src_box_present"] else "footprint only"] += 1
        source_formats[record["src_format"]] += 1
        cookies.add(record["cookie"])

        if original not in EXPECTED_TRANSFORMS:
            errors.append(f"sequence {record['sequence']} has unexpected original extent {original}")
        elif emitted != EXPECTED_TRANSFORMS[original]:
            errors.append(
                f"sequence {record['sequence']} emitted {emitted}, expected {EXPECTED_TRANSFORMS[original]}"
            )
        if record["src_format"] != 0x3:
            errors.append(f"sequence {record['sequence']} has unexpected source format {record['src_format']:#x}")
        if record["dst_format"] != 0x4D:
            errors.append(f"sequence {record['sequence']} has unexpected destination format {record['dst_format']:#x}")
        if record["ix"] < 0 or record["iy"] < 0 or record["iz"] != 0:
            errors.append(f"sequence {record['sequence']} has an invalid destination offset")
        if record["ix"] % 4 or record["iy"] % 4:
            errors.append(f"sequence {record['sequence']} has a non-block-aligned destination offset")
        if record["src_box_present"] or record["sl"] or record["st"] or record["sf"]:
            errors.append(f"sequence {record['sequence']} is not the observed footprint-only source form")
        if ((record["sw"], record["sh"], record["sd"]) != original or
                (record["fw"], record["fh"], record["fd"]) != original):
            errors.append(f"sequence {record['sequence']} source footprint does not equal the original extent")
        if record["ix"] + record["ew"] > 2048 or record["iy"] + record["eh"] > 2048:
            errors.append(f"sequence {record['sequence']} exceeds the destination mip")
        expected_row_length = record["row_pitch"] // 16 * 4
        expected_image_height = record["fh"] * 4
        if record["row_pitch"] % 16 or record["row_pitch"] < record["fw"] * 16:
            errors.append(f"sequence {record['sequence']} source footprint lacks complete 128-bit elements")
        if record["buffer_row_length"] != expected_row_length:
            errors.append(
                f"sequence {record['sequence']} has bufferRowLength {record['buffer_row_length']}, "
                f"expected {expected_row_length} BC3 texels"
            )
        if record["buffer_image_height"] != expected_image_height:
            errors.append(
                f"sequence {record['sequence']} has bufferImageHeight {record['buffer_image_height']}, "
                f"expected {expected_image_height} BC3 texels"
            )

    if adjustments and len(adjustments) != 432:
        warnings.append(
            f"D05c logged {len(adjustments)} adjustments; D02/D05b logged 432, but scene duration and cache demand can change the count"
        )

    status = "valid" if not errors else "invalid"
    lines = [
        "# D05 BC3 border-copy analysis",
        "",
        f"- Instrumentation validity: **{status}**",
        f"- Enable markers: {enabled_count}",
        f"- Target-class candidates: {len(candidates)}",
        f"- Logged adjustments: {len(adjustments)}",
        f"- Rejected candidates: {len(rejections)}",
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

    lines.extend(["", "## Source representation", "", "| Form | Count |", "|---|---:|"])
    for source_form, count in sorted(source_forms.items()):
        lines.append(f"| {source_form} | {count} |")
    lines.extend(["", "| Source DXGI format | Count |", "|---|---:|"])
    for source_format, count in sorted(source_formats.items()):
        lines.append(f"| `{source_format:#x}` | {count} |")

    lines.extend(["", "## Source footprints", "", "| Width x height x depth | Row pitch | Count |", "|---|---:|---:|"])
    for (width, height, depth, row_pitch), count in sorted(footprints.items()):
        lines.append(f"| {width}x{height}x{depth} | {row_pitch} | {count} |")
    if rejection_masks:
        lines.extend(["", "## Rejection masks", "", "| Mask | Count |", "|---|---:|"])
        for mask, count in sorted(rejection_masks.items()):
            lines.append(f"| `{mask:#x}` | {count} |")

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
        "A valid result proves that D05c recognized only the observed Korea terrain-cache border class, "
        "mapped every 128-bit R32G32B32A32_UINT source element to one 4x4 BC3 destination block, "
        "and expressed the Vulkan extent and buffer layout in destination-format texels while remaining inside the 2048x2048 mip. "
        "Visual classification is still required to decide whether this compatibility behavior affects seams, "
        "missing pages, both, or neither.",
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
