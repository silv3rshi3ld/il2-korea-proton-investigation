#!/usr/bin/env python3

import struct
import sys


SPIRV_MAGIC = 0x07230203
OP_LOGICAL_OR = 166
OP_LOGICAL_AND = 167
OP_F_ORD_LESS_THAN = 184
OP_F_ORD_GREATER_THAN_EQUAL = 190
OP_PHI = 245
OP_SELECTION_MERGE = 247
OP_BRANCH_CONDITIONAL = 250


def fail(message):
    raise SystemExit(message)


def parse_id(text, name):
    try:
        value = int(text, 0)
    except ValueError:
        fail("invalid %s: %s" % (name, text))
    if value <= 0 or value > 0xFFFFFFFF:
        fail("invalid %s: %s" % (name, text))
    return value


def instruction(opcode, *operands):
    return [((len(operands) + 1) << 16) | opcode, *operands]


def read_module(path):
    data = open(path, "rb").read()
    if len(data) < 20 or len(data) % 4:
        fail("%s is not a complete SPIR-V word stream" % path)
    words = list(struct.unpack("<%dI" % (len(data) // 4), data))
    if words[0] != SPIRV_MAGIC:
        fail("%s has unexpected SPIR-V magic" % path)
    return words


def write_module(path, words):
    with open(path, "wb") as output:
        output.write(struct.pack("<%dI" % len(words), *words))


def find_contract(words, contract):
    matches = {
        "near": 0,
        "far": 0,
        "membership": 0,
        "merge": 0,
        "branch": 0,
    }
    membership_end = None
    merge_offset = None
    branch_offset = None
    offset = 5
    while offset < len(words):
        word_count = words[offset] >> 16
        opcode = words[offset] & 0xFFFF
        if not word_count or offset + word_count > len(words):
            fail("malformed instruction at word %d" % offset)
        operands = words[offset + 1 : offset + word_count]
        if opcode == OP_PHI and len(operands) >= 4:
            if operands[0] == contract["float_type"] and operands[1] == contract["near"]:
                matches["near"] += 1
            elif operands[0] == contract["float_type"] and operands[1] == contract["far"]:
                matches["far"] += 1
            elif operands[0] == contract["bool_type"] and operands[1] == contract["membership"]:
                matches["membership"] += 1
                membership_end = offset + word_count
        elif opcode == OP_SELECTION_MERGE and operands == [contract["merge"], 0]:
            matches["merge"] += 1
            merge_offset = offset
        elif opcode == OP_BRANCH_CONDITIONAL and operands == [
            contract["membership"],
            contract["true_label"],
            contract["false_label"],
        ]:
            matches["branch"] += 1
            branch_offset = offset
        offset += word_count

    adjacent = (
        membership_end is not None
        and membership_end == merge_offset
        and merge_offset + 3 == branch_offset
    )
    if any(value != 1 for value in matches.values()) or not adjacent:
        fail(
            "input contract mismatch: %s adjacency=%s"
            % (" ".join("%s=%d" % item for item in matches.items()), "yes" if adjacent else "no")
        )
    return merge_offset, branch_offset


def main():
    if len(sys.argv) != 15:
        fail(
            "usage: %s INPUT OUTPUT BOOL_TYPE FLOAT_TYPE NEAR FAR TILE_MIN "
            "TILE_MAX MEMBERSHIP MERGE TRUE FALSE VERSION GUARD" % sys.argv[0]
        )

    contract = {
        "bool_type": parse_id(sys.argv[3], "boolean type"),
        "float_type": parse_id(sys.argv[4], "float type"),
        "near": parse_id(sys.argv[5], "near value"),
        "far": parse_id(sys.argv[6], "far value"),
        "tile_min": parse_id(sys.argv[7], "tile-min value"),
        "tile_max": parse_id(sys.argv[8], "tile-max value"),
        "membership": parse_id(sys.argv[9], "membership"),
        "merge": parse_id(sys.argv[10], "merge label"),
        "true_label": parse_id(sys.argv[11], "true label"),
        "false_label": parse_id(sys.argv[12], "false label"),
    }
    if parse_id(sys.argv[13], "contract version") != 1 or parse_id(sys.argv[14], "guard") != 39:
        fail("unsupported contract")

    words = read_module(sys.argv[1])
    merge_offset, branch_offset = find_contract(words, contract)
    old_bound = words[3]
    if old_bound <= 0 or old_bound > 0xFFFFFFFF - 9:
        fail("cannot allocate nine result IDs")

    ids = list(range(old_bound, old_bound + 9))
    valid, near_ge_min, near_lt_max, near_inside = ids[0:4]
    min_ge_near, min_lt_far, min_inside, interval_overlap, final = ids[4:9]
    bool_type = contract["bool_type"]
    inserted = []
    inserted += instruction(OP_F_ORD_LESS_THAN, bool_type, valid, contract["near"], contract["far"])
    inserted += instruction(OP_F_ORD_GREATER_THAN_EQUAL, bool_type, near_ge_min, contract["near"], contract["tile_min"])
    inserted += instruction(OP_F_ORD_LESS_THAN, bool_type, near_lt_max, contract["near"], contract["tile_max"])
    inserted += instruction(OP_LOGICAL_AND, bool_type, near_inside, near_ge_min, near_lt_max)
    inserted += instruction(OP_F_ORD_GREATER_THAN_EQUAL, bool_type, min_ge_near, contract["tile_min"], contract["near"])
    inserted += instruction(OP_F_ORD_LESS_THAN, bool_type, min_lt_far, contract["tile_min"], contract["far"])
    inserted += instruction(OP_LOGICAL_AND, bool_type, min_inside, min_ge_near, min_lt_far)
    inserted += instruction(OP_LOGICAL_OR, bool_type, interval_overlap, near_inside, min_inside)
    inserted += instruction(OP_LOGICAL_AND, bool_type, final, valid, interval_overlap)

    output_words = words[:merge_offset] + inserted + words[merge_offset:]
    output_words[3] = old_bound + 9
    output_branch = branch_offset + len(inserted)
    if (
        output_words[output_branch] != ((4 << 16) | OP_BRANCH_CONDITIONAL)
        or output_words[output_branch + 1] != contract["membership"]
    ):
        fail("output branch contract changed unexpectedly")
    output_words[output_branch + 1] = final
    write_module(sys.argv[2], output_words)
    print(
        "SCALAR_DEPTH_OVERLAP_PATCH input=%s output=%s original=%d final=%d "
        "old_bound=%d new_bound=%d"
        % (sys.argv[1], sys.argv[2], contract["membership"], final, old_bound, old_bound + 9)
    )


if __name__ == "__main__":
    main()
