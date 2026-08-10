#!/usr/bin/env python3
"""Compare IL-2's tiled-light grid and index list across captured frames.

The 80x34x2 R32_UINT grid stores, per tile:

* layer 0: low 10 bits = light count, upper bits = first index;
* layer 1: upper bits = final index cursor.

The companion 87,040-byte buffer contains 43,520 R16_UINT light IDs.
"""

import argparse
import hashlib
import struct
from dataclasses import dataclass
from pathlib import Path


WIDTH = 80
HEIGHT = 34
TILES = WIDTH * HEIGHT
GROUP_WIDTH = 8
GROUP_HEIGHT = 8
COUNT_MASK = (1 << 10) - 1


@dataclass(frozen=True)
class Frame:
    label: str
    grid_path: Path
    indices_path: Path
    grid_sha256: str
    indices_sha256: str
    counts: tuple[int, ...]
    starts: tuple[int, ...]
    ends: tuple[int, ...]
    indices: tuple[int, ...]


def read_frame(spec: str) -> Frame:
    try:
        label, grid_name, indices_name = spec.split("=", 2)
    except ValueError as exc:
        raise ValueError(
            "frame must be LABEL=GRID.bin=INDICES.bin: %r" % spec
        ) from exc

    grid_path = Path(grid_name)
    indices_path = Path(indices_name)
    grid_raw = grid_path.read_bytes()
    indices_raw = indices_path.read_bytes()
    expected_grid_size = TILES * 2 * 4
    if len(grid_raw) != expected_grid_size:
        raise ValueError(
            "%s: expected %d grid bytes, got %d"
            % (label, expected_grid_size, len(grid_raw))
        )
    if len(indices_raw) % 2:
        raise ValueError("%s: index buffer has an odd byte count" % label)

    grid = struct.unpack("<%dI" % (TILES * 2), grid_raw)
    layer0 = grid[:TILES]
    layer1 = grid[TILES:]
    counts = tuple(value & COUNT_MASK for value in layer0)
    starts = tuple(value >> 10 for value in layer0)
    ends = tuple(value >> 10 for value in layer1)
    indices = struct.unpack("<%dH" % (len(indices_raw) // 2), indices_raw)

    return Frame(
        label=label,
        grid_path=grid_path,
        indices_path=indices_path,
        grid_sha256=hashlib.sha256(grid_raw).hexdigest(),
        indices_sha256=hashlib.sha256(indices_raw).hexdigest(),
        counts=counts,
        starts=starts,
        ends=ends,
        indices=indices,
    )


def workgroup_intervals(frame: Frame, group_x: int, group_y: int):
    result = []
    for local_y in range(GROUP_HEIGHT):
        y = group_y * GROUP_HEIGHT + local_y
        if y >= HEIGHT:
            continue
        for local_x in range(GROUP_WIDTH):
            x = group_x * GROUP_WIDTH + local_x
            if x >= WIDTH:
                continue
            tile = y * WIDTH + x
            result.append(
                (frame.starts[tile], frame.ends[tile], frame.counts[tile], x, y)
            )
    return result


def is_gap_free_from_zero(intervals) -> bool:
    cursor = 0
    for start, end, count, _x, _y in sorted(intervals):
        if start != cursor or end != start + count:
            return False
        cursor = end
    return True


def summarize(frame: Frame):
    groups_x = (WIDTH + GROUP_WIDTH - 1) // GROUP_WIDTH
    groups_y = (HEIGHT + GROUP_HEIGHT - 1) // GROUP_HEIGHT
    groups = [
        workgroup_intervals(frame, group_x, group_y)
        for group_y in range(groups_y)
        for group_x in range(groups_x)
    ]
    gap_free = sum(is_gap_free_from_zero(group) for group in groups)
    end_mismatches = sum(
        end != start + count
        for start, end, count in zip(frame.starts, frame.ends, frame.counts)
    )
    max_end = max(frame.ends)
    used = frame.indices[:max_end]
    nonzero_inside = sum(value != 0 for value in used)
    nonzero_outside = sum(value != 0 for value in frame.indices[max_end:])

    print("frame=%s" % frame.label)
    print("  grid=%s" % frame.grid_path)
    print("  grid_sha256=%s" % frame.grid_sha256)
    print("  indices=%s" % frame.indices_path)
    print("  indices_sha256=%s" % frame.indices_sha256)
    print(
        "  tiles=%d requested=%d count_min=%d count_max=%d"
        % (TILES, sum(frame.counts), min(frame.counts), max(frame.counts))
    )
    print(
        "  start_max=%d end_max=%d end_mismatches=%d"
        % (max(frame.starts), max_end, end_mismatches)
    )
    print(
        "  workgroups=%d gap_free_from_zero=%d"
        % (len(groups), gap_free)
    )
    print(
        "  index_entries=%d used_prefix=%d nonzero_inside=%d nonzero_outside=%d"
        % (len(frame.indices), max_end, nonzero_inside, nonzero_outside)
    )


def compare(left: Frame, right: Frame):
    metadata_differences = sum(
        a != b
        for a, b in zip(
            zip(left.counts, left.starts, left.ends),
            zip(right.counts, right.starts, right.ends),
        )
    )
    index_differences = sum(a != b for a, b in zip(left.indices, right.indices))
    common_prefix = min(max(left.ends), max(right.ends))
    prefix_differences = sum(
        a != b
        for a, b in zip(left.indices[:common_prefix], right.indices[:common_prefix])
    )
    print("compare=%s:%s" % (left.label, right.label))
    print("  metadata_tile_differences=%d" % metadata_differences)
    print("  all_index_differences=%d" % index_differences)
    print(
        "  used_prefix=%d used_prefix_differences=%d"
        % (common_prefix, prefix_differences)
    )


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "frame",
        nargs="+",
        help="LABEL=GRID.bin=INDICES.bin (supply two or more to compare)",
    )
    args = parser.parse_args()
    frames = [read_frame(spec) for spec in args.frame]
    for frame in frames:
        summarize(frame)
    for index, left in enumerate(frames):
        for right in frames[index + 1 :]:
            compare(left, right)


if __name__ == "__main__":
    main()
