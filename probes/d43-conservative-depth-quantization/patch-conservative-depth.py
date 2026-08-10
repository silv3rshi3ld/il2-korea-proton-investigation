#!/usr/bin/env python3

import struct
import sys


SPIRV_MAGIC = 0x07230203
OP_IADD = 128
OP_ISUB = 130
OP_LOGICAL_AND = 167
OP_SELECT = 169
OP_I_NOT_EQUAL = 171
OP_U_LESS_THAN = 176
OP_SHIFT_RIGHT_LOGICAL = 194
OP_SHIFT_LEFT_LOGICAL = 196
OP_BITWISE_OR = 197

OP_SHIFT_FAR = 196
OP_PACK_DEPTH = 197
OP_COMPOSITE_CONSTRUCT = 80


def fail(message):
    raise SystemExit(message)


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


def find_instruction(words, opcode, operands):
    matches = []
    offset = 5
    while offset < len(words):
        word_count = words[offset] >> 16
        current_opcode = words[offset] & 0xffff
        if not word_count or offset + word_count > len(words):
            fail("malformed instruction at word %d" % offset)
        if current_opcode == opcode and words[offset + 1:offset + word_count] == operands:
            matches.append(offset)
        offset += word_count
    if len(matches) != 1:
        fail("instruction contract mismatch for opcode %d: matches=%d" %
             (opcode, len(matches)))
    return matches[0]


def main():
    if len(sys.argv) != 3:
        fail("usage: %s INPUT OUTPUT" % sys.argv[0])

    words = read_module(sys.argv[1])
    # Exact translated ComputeDepthRange contract for e41c75bf472dc42b.
    uint_type = 6
    bool_type = 70
    zero = 34
    one = 37
    mantissa_max = 155
    shift_16 = 230
    near_mantissa = 154
    near_packed = 160
    depth_mask = 96
    far_mantissa = 226
    far_packed = 228
    far_shifted = 229
    depth_packed = 231
    vector_type = 234
    output_value = 235

    far_shift_offset = find_instruction(
        words,
        OP_SHIFT_FAR,
        [uint_type, far_shifted, far_packed, shift_16],
    )
    depth_pack_offset = find_instruction(
        words,
        OP_PACK_DEPTH,
        [uint_type, depth_packed, near_packed, far_shifted],
    )
    output_offset = find_instruction(
        words,
        OP_COMPOSITE_CONSTRUCT,
        [vector_type, output_value, depth_packed, depth_mask, depth_packed, depth_packed],
    )
    if not (far_shift_offset < depth_pack_offset < output_offset):
        fail("depth output contract has unexpected instruction order")

    old_bound = words[3]
    if old_bound <= 0 or old_bound > 0xffffffff - 12:
        fail("cannot allocate twelve result IDs")
    ids = list(range(old_bound, old_bound + 12))
    (
        near_nonzero,
        near_decremented,
        near_conservative,
        far_nonzero,
        far_below_max,
        far_can_increment,
        far_incremented,
        far_conservative,
        mask_left,
        mask_right,
        mask_partial,
        mask_conservative,
    ) = ids

    inserted = []
    inserted += instruction(OP_I_NOT_EQUAL, bool_type, near_nonzero, near_mantissa, zero)
    inserted += instruction(OP_ISUB, uint_type, near_decremented, near_packed, one)
    inserted += instruction(OP_SELECT, uint_type, near_conservative,
                            near_nonzero, near_decremented, near_packed)
    inserted += instruction(OP_I_NOT_EQUAL, bool_type, far_nonzero, far_mantissa, zero)
    inserted += instruction(OP_U_LESS_THAN, bool_type, far_below_max,
                            far_mantissa, mantissa_max)
    inserted += instruction(OP_LOGICAL_AND, bool_type, far_can_increment,
                            far_nonzero, far_below_max)
    inserted += instruction(OP_IADD, uint_type, far_incremented, far_packed, one)
    inserted += instruction(OP_SELECT, uint_type, far_conservative,
                            far_can_increment, far_incremented, far_packed)
    inserted += instruction(OP_SHIFT_LEFT_LOGICAL, uint_type, mask_left, depth_mask, one)
    inserted += instruction(OP_SHIFT_RIGHT_LOGICAL, uint_type, mask_right, depth_mask, one)
    inserted += instruction(OP_BITWISE_OR, uint_type, mask_partial, depth_mask, mask_left)
    inserted += instruction(OP_BITWISE_OR, uint_type, mask_conservative,
                            mask_partial, mask_right)

    output_words = words[:far_shift_offset] + inserted + words[far_shift_offset:]
    output_words[3] = old_bound + 12
    inserted_count = len(inserted)

    shifted_far_offset = far_shift_offset + inserted_count
    shifted_depth_offset = depth_pack_offset + inserted_count
    shifted_output_offset = output_offset + inserted_count
    if output_words[shifted_far_offset + 3] != far_packed:
        fail("far-pack operand changed unexpectedly")
    if output_words[shifted_depth_offset + 3] != near_packed:
        fail("near-pack operand changed unexpectedly")
    if output_words[shifted_output_offset + 4] != depth_mask:
        fail("mask output operand changed unexpectedly")
    output_words[shifted_far_offset + 3] = far_conservative
    output_words[shifted_depth_offset + 3] = near_conservative
    output_words[shifted_output_offset + 4] = mask_conservative

    write_module(sys.argv[2], output_words)
    print(
        "CONSERVATIVE_DEPTH_PATCH input=%s output=%s old_bound=%d new_bound=%d "
        "near=%d far=%d mask=%d"
        % (
            sys.argv[1],
            sys.argv[2],
            old_bound,
            old_bound + 12,
            near_conservative,
            far_conservative,
            mask_conservative,
        )
    )


if __name__ == "__main__":
    main()
