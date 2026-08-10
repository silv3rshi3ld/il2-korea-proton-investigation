#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SPIRV_MAGIC 0x07230203u
#define SPV_OP_CONSTANT 43u
#define SPV_OP_LOAD 61u
#define SPV_OP_ACCESS_CHAIN 65u
#define SPV_OP_LOGICAL_OR 166u
#define SPV_OP_I_EQUAL 170u
#define SPV_OP_PHI 245u
#define SPV_OP_SELECTION_MERGE 247u
#define SPV_OP_BRANCH_CONDITIONAL 250u

struct patch_contract
{
    uint32_t uint_type;
    uint32_t bool_type;
    uint32_t global_id;
    uint32_t light_id;
    uint32_t record2_constant;
    uint32_t membership;
    uint32_t merge_label;
    uint32_t true_label;
    uint32_t false_label;
};

static uint32_t parse_id(const char *text, const char *name)
{
    char *end;
    unsigned long value;

    errno = 0;
    value = strtoul(text, &end, 0);
    if (errno || !*text || *end || !value || value > UINT32_MAX)
    {
        fprintf(stderr, "Invalid %s: %s\n", name, text);
        exit(EXIT_FAILURE);
    }
    return (uint32_t)value;
}

static uint32_t *read_module(const char *path, size_t *word_count)
{
    uint32_t *words;
    long size;
    FILE *file;

    if (!(file = fopen(path, "rb")))
    {
        fprintf(stderr, "Cannot open %s: %s\n", path, strerror(errno));
        exit(EXIT_FAILURE);
    }
    if (fseek(file, 0, SEEK_END) || (size = ftell(file)) < 0 ||
            fseek(file, 0, SEEK_SET))
    {
        fprintf(stderr, "Cannot size %s: %s\n", path, strerror(errno));
        fclose(file);
        exit(EXIT_FAILURE);
    }
    if ((size_t)size < 5 * sizeof(uint32_t) || size % sizeof(uint32_t))
    {
        fprintf(stderr, "%s is not a complete SPIR-V word stream\n", path);
        fclose(file);
        exit(EXIT_FAILURE);
    }
    if (!(words = malloc((size_t)size)))
    {
        fprintf(stderr, "Out of memory reading %s\n", path);
        fclose(file);
        exit(EXIT_FAILURE);
    }
    if (fread(words, 1, (size_t)size, file) != (size_t)size)
    {
        fprintf(stderr, "Cannot read %s: %s\n", path, strerror(errno));
        free(words);
        fclose(file);
        exit(EXIT_FAILURE);
    }
    fclose(file);
    if (words[0] != SPIRV_MAGIC)
    {
        fprintf(stderr, "%s has unexpected SPIR-V magic 0x%08"PRIx32"\n",
                path, words[0]);
        free(words);
        exit(EXIT_FAILURE);
    }
    *word_count = (size_t)size / sizeof(uint32_t);
    return words;
}

static void write_module(const char *path, const uint32_t *words,
        size_t word_count)
{
    FILE *file;

    if (!(file = fopen(path, "wb")))
    {
        fprintf(stderr, "Cannot create %s: %s\n", path, strerror(errno));
        exit(EXIT_FAILURE);
    }
    if (fwrite(words, sizeof(*words), word_count, file) != word_count ||
            fclose(file))
    {
        fprintf(stderr, "Cannot write %s: %s\n", path, strerror(errno));
        exit(EXIT_FAILURE);
    }
}

static void inspect_instruction(const uint32_t *words, size_t word_count,
        size_t offset, uint32_t *instruction_words, uint32_t *opcode)
{
    *instruction_words = words[offset] >> 16;
    *opcode = words[offset] & 0xffffu;
    if (!*instruction_words || offset + *instruction_words > word_count)
    {
        fprintf(stderr, "Malformed instruction at word %zu\n", offset);
        exit(EXIT_FAILURE);
    }
}

static void find_contract(const uint32_t *words, size_t word_count,
        const struct patch_contract *contract, size_t *merge_offset,
        size_t *branch_offset)
{
    size_t offset;
    size_t access_matches = 0;
    size_t load_matches = 0;
    size_t constant_matches = 0;
    size_t phi_matches = 0;
    size_t merge_matches = 0;
    size_t branch_matches = 0;
    size_t phi_end = 0;
    uint32_t light_pointer = 0;

    for (offset = 5; offset < word_count;)
    {
        uint32_t instruction_words;
        uint32_t opcode;

        inspect_instruction(words, word_count, offset, &instruction_words, &opcode);
        if (instruction_words == 5 && opcode == SPV_OP_ACCESS_CHAIN &&
                words[offset + 3] == contract->global_id &&
                words[offset + 4] == contract->record2_constant)
        {
            light_pointer = words[offset + 2];
            ++access_matches;
        }
        else if (instruction_words == 4 && opcode == SPV_OP_LOAD &&
                words[offset + 1] == contract->uint_type &&
                words[offset + 2] == contract->light_id &&
                words[offset + 3] == light_pointer)
            ++load_matches;
        else if (instruction_words == 4 && opcode == SPV_OP_CONSTANT &&
                words[offset + 1] == contract->uint_type &&
                words[offset + 2] == contract->record2_constant &&
                words[offset + 3] == 2)
            ++constant_matches;
        else if (instruction_words >= 7 && opcode == SPV_OP_PHI &&
                words[offset + 1] == contract->bool_type &&
                words[offset + 2] == contract->membership)
        {
            ++phi_matches;
            phi_end = offset + instruction_words;
        }
        else if (instruction_words == 3 && opcode == SPV_OP_SELECTION_MERGE &&
                words[offset + 1] == contract->merge_label)
        {
            ++merge_matches;
            *merge_offset = offset;
        }
        else if (instruction_words == 4 && opcode == SPV_OP_BRANCH_CONDITIONAL &&
                words[offset + 1] == contract->membership &&
                words[offset + 2] == contract->true_label &&
                words[offset + 3] == contract->false_label)
        {
            ++branch_matches;
            *branch_offset = offset;
        }
        offset += instruction_words;
    }

    if (access_matches != 1 || load_matches != 1 || constant_matches != 1 ||
            phi_matches != 1 || merge_matches != 1 || branch_matches != 1 ||
            phi_end != *merge_offset || *merge_offset + 3 != *branch_offset)
    {
        fprintf(stderr, "Input contract mismatch: access=%zu load=%zu "
                "constant=%zu phi=%zu merge=%zu branch=%zu adjacency=%s\n",
                access_matches, load_matches, constant_matches, phi_matches,
                merge_matches, branch_matches, phi_end == *merge_offset &&
                *merge_offset + 3 == *branch_offset ? "yes" : "no");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char **argv)
{
    struct patch_contract contract;
    uint32_t *input_words;
    uint32_t *output_words;
    uint32_t equality_id;
    uint32_t inclusive_membership_id;
    uint32_t old_bound;
    size_t input_count;
    size_t output_count;
    size_t merge_offset = 0;
    size_t branch_offset = 0;
    size_t output_branch_offset;

    if (argc != 13)
    {
        fprintf(stderr, "Usage: %s INPUT.spv OUTPUT.spv UINT_TYPE BOOL_TYPE "
                "GLOBAL_ID LIGHT_ID RECORD2_CONSTANT MEMBERSHIP MERGE_LABEL "
                "TRUE_LABEL FALSE_LABEL CONTRACT_VERSION\n", argv[0]);
        return EXIT_FAILURE;
    }

    contract.uint_type = parse_id(argv[3], "uint type ID");
    contract.bool_type = parse_id(argv[4], "boolean type ID");
    contract.global_id = parse_id(argv[5], "global invocation ID");
    contract.light_id = parse_id(argv[6], "light ID");
    contract.record2_constant = parse_id(argv[7], "record-2 constant ID");
    contract.membership = parse_id(argv[8], "membership ID");
    contract.merge_label = parse_id(argv[9], "merge label ID");
    contract.true_label = parse_id(argv[10], "true label ID");
    contract.false_label = parse_id(argv[11], "false label ID");
    if (parse_id(argv[12], "contract version") != 1)
    {
        fprintf(stderr, "Unsupported contract version\n");
        return EXIT_FAILURE;
    }

    input_words = read_module(argv[1], &input_count);
    find_contract(input_words, input_count, &contract, &merge_offset,
            &branch_offset);

    old_bound = input_words[3];
    if (!old_bound || old_bound > UINT32_MAX - 2)
    {
        fprintf(stderr, "Cannot allocate two IDs from bound %"PRIu32"\n",
                old_bound);
        free(input_words);
        return EXIT_FAILURE;
    }
    equality_id = old_bound;
    inclusive_membership_id = old_bound + 1;
    output_count = input_count + 10;
    if (!(output_words = malloc(output_count * sizeof(*output_words))))
    {
        fprintf(stderr, "Out of memory creating output module\n");
        free(input_words);
        return EXIT_FAILURE;
    }

    memcpy(output_words, input_words, merge_offset * sizeof(*output_words));
    output_words[merge_offset + 0] = (5u << 16) | SPV_OP_I_EQUAL;
    output_words[merge_offset + 1] = contract.bool_type;
    output_words[merge_offset + 2] = equality_id;
    output_words[merge_offset + 3] = contract.light_id;
    output_words[merge_offset + 4] = contract.record2_constant;
    output_words[merge_offset + 5] = (5u << 16) | SPV_OP_LOGICAL_OR;
    output_words[merge_offset + 6] = contract.bool_type;
    output_words[merge_offset + 7] = inclusive_membership_id;
    output_words[merge_offset + 8] = contract.membership;
    output_words[merge_offset + 9] = equality_id;
    memcpy(&output_words[merge_offset + 10], &input_words[merge_offset],
            (input_count - merge_offset) * sizeof(*output_words));

    output_words[3] = old_bound + 2;
    output_branch_offset = branch_offset + 10;
    if ((output_words[output_branch_offset] >> 16) != 4 ||
            (output_words[output_branch_offset] & 0xffffu) !=
            SPV_OP_BRANCH_CONDITIONAL ||
            output_words[output_branch_offset + 1] != contract.membership)
    {
        fprintf(stderr, "Output branch contract changed unexpectedly\n");
        free(output_words);
        free(input_words);
        return EXIT_FAILURE;
    }
    output_words[output_branch_offset + 1] = inclusive_membership_id;

    write_module(argv[2], output_words, output_count);
    printf("GLOBAL_RECORD2_PRODUCER_PATCH input=%s output=%s light_id=%"PRIu32
            " original_membership=%"PRIu32" inclusive_membership=%"PRIu32
            " old_bound=%"PRIu32" new_bound=%"PRIu32"\n",
            argv[1], argv[2], contract.light_id, contract.membership,
            inclusive_membership_id, old_bound, old_bound + 2);
    free(output_words);
    free(input_words);
    return EXIT_SUCCESS;
}
