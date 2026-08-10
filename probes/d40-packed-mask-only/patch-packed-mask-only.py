#!/usr/bin/env python3

import importlib.util
from pathlib import Path
import sys


BASE_PATH = (
    Path(__file__).resolve().parents[1]
    / "d39-scalar-depth-overlap"
    / "patch-scalar-depth-overlap.py"
)
SPEC = importlib.util.spec_from_file_location("d39_spirv_patch", BASE_PATH)
base = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(base)

OP_LOGICAL_NOT = 168


def main():
    if len(sys.argv) != 14:
        base.fail(
            "usage: %s INPUT OUTPUT BOOL_TYPE FLOAT_TYPE NEAR FAR MASK_ZERO "
            "MEMBERSHIP MERGE TRUE FALSE VERSION GUARD" % sys.argv[0]
        )

    contract = {
        "bool_type": base.parse_id(sys.argv[3], "boolean type"),
        "float_type": base.parse_id(sys.argv[4], "float type"),
        "near": base.parse_id(sys.argv[5], "near value"),
        "far": base.parse_id(sys.argv[6], "far value"),
        "mask_zero": base.parse_id(sys.argv[7], "mask-zero predicate"),
        "membership": base.parse_id(sys.argv[8], "membership"),
        "merge": base.parse_id(sys.argv[9], "merge label"),
        "true_label": base.parse_id(sys.argv[10], "true label"),
        "false_label": base.parse_id(sys.argv[11], "false label"),
    }
    if base.parse_id(sys.argv[12], "contract version") != 1 or base.parse_id(sys.argv[13], "guard") != 40:
        base.fail("unsupported contract")

    words = base.read_module(sys.argv[1])
    merge_offset, branch_offset = base.find_contract(words, contract)
    old_bound = words[3]
    if old_bound <= 0 or old_bound > 0xFFFFFFFF - 3:
        base.fail("cannot allocate three result IDs")

    valid = old_bound
    mask_overlap = old_bound + 1
    final = old_bound + 2
    bool_type = contract["bool_type"]
    inserted = []
    inserted += base.instruction(
        base.OP_F_ORD_LESS_THAN,
        bool_type,
        valid,
        contract["near"],
        contract["far"],
    )
    inserted += base.instruction(
        OP_LOGICAL_NOT,
        bool_type,
        mask_overlap,
        contract["mask_zero"],
    )
    inserted += base.instruction(
        base.OP_LOGICAL_AND,
        bool_type,
        final,
        valid,
        mask_overlap,
    )

    output_words = words[:merge_offset] + inserted + words[merge_offset:]
    output_words[3] = old_bound + 3
    output_branch = branch_offset + len(inserted)
    if (
        output_words[output_branch]
        != ((4 << 16) | base.OP_BRANCH_CONDITIONAL)
        or output_words[output_branch + 1] != contract["membership"]
    ):
        base.fail("output branch contract changed unexpectedly")
    output_words[output_branch + 1] = final
    base.write_module(sys.argv[2], output_words)
    print(
        "PACKED_MASK_ONLY_PATCH input=%s output=%s original=%d final=%d "
        "old_bound=%d new_bound=%d"
        % (
            sys.argv[1],
            sys.argv[2],
            contract["membership"],
            final,
            old_bound,
            old_bound + 3,
        )
    )


if __name__ == "__main__":
    main()
