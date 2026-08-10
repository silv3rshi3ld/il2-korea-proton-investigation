#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SPIRV_MAGIC 0x07230203u
#define SPV_OP_COMPOSITE_EXTRACT 81u
#define SPV_OP_SELECT 169u
#define SPV_OP_I_EQUAL 170u
#define SPV_OP_SHIFT_LEFT_LOGICAL 196u

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

static size_t find_extract(const uint32_t *words, size_t module_words,
        uint32_t type_id, uint32_t result_id, uint32_t composite_id)
{
    size_t offset, match_offset = 0, matches = 0;

    for (offset = 5; offset < module_words;)
    {
        uint32_t instruction = words[offset];
        uint32_t instruction_words = instruction >> 16;
        uint32_t opcode = instruction & 0xffffu;

        if (!instruction_words || offset + instruction_words > module_words)
        {
            fprintf(stderr, "Malformed instruction at word %zu\n", offset);
            exit(EXIT_FAILURE);
        }
        if (instruction_words == 5 && opcode == SPV_OP_COMPOSITE_EXTRACT &&
                words[offset + 1] == type_id && words[offset + 2] == result_id &&
                words[offset + 3] == composite_id && words[offset + 4] == 0)
        {
            match_offset = offset;
            ++matches;
        }
        offset += instruction_words;
    }
    if (matches != 1)
    {
        fprintf(stderr, "Expected one light-ID extract, found %zu\n", matches);
        exit(EXIT_FAILURE);
    }
    return match_offset;
}

int main(int argc, char **argv)
{
    uint32_t type_id, extract_id, composite_id, zero_id, selected_id;
    uint32_t fallback_id, shift_id, bool_type_id, condition_id, filtered_id;
    uint32_t *input_words, *output_words;
    size_t input_count, output_count, extract_offset, insert_offset, offset;
    size_t zero_matches = 0, shift_matches = 0;

    if (argc != 11)
    {
        fprintf(stderr, "Usage: %s INPUT.spv OUTPUT.spv TYPE_ID EXTRACT_ID "
                "COMPOSITE_ID ZERO_ID SELECTED_ID FALLBACK_ID SHIFT_ID "
                "BOOL_TYPE_ID\n", argv[0]);
        return EXIT_FAILURE;
    }

    type_id = parse_id(argv[3], "result type ID");
    extract_id = parse_id(argv[4], "extracted light ID");
    composite_id = parse_id(argv[5], "fetch composite ID");
    zero_id = parse_id(argv[6], "zero constant ID");
    selected_id = parse_id(argv[7], "selected light-ID constant");
    fallback_id = parse_id(argv[8], "fallback light-ID constant");
    shift_id = parse_id(argv[9], "light-record shift constant ID");
    bool_type_id = parse_id(argv[10], "boolean type ID");

    input_words = read_module(argv[1], &input_count);
    extract_offset = find_extract(input_words, input_count,
            type_id, extract_id, composite_id);
    insert_offset = extract_offset + 5;
    condition_id = input_words[3];
    filtered_id = condition_id + 1;
    if (!condition_id || condition_id >= UINT32_MAX - 1)
    {
        fprintf(stderr, "Cannot allocate two IDs from bound %"PRIu32"\n",
                condition_id);
        free(input_words);
        return EXIT_FAILURE;
    }

    output_count = input_count + 11;
    if (!(output_words = malloc(output_count * sizeof(*output_words))))
    {
        fprintf(stderr, "Out of memory creating output module\n");
        free(input_words);
        return EXIT_FAILURE;
    }

    memcpy(output_words, input_words, insert_offset * sizeof(*output_words));
    output_words[insert_offset + 0] = (5u << 16) | SPV_OP_I_EQUAL;
    output_words[insert_offset + 1] = bool_type_id;
    output_words[insert_offset + 2] = condition_id;
    output_words[insert_offset + 3] = extract_id;
    output_words[insert_offset + 4] = selected_id;
    output_words[insert_offset + 5] = (6u << 16) | SPV_OP_SELECT;
    output_words[insert_offset + 6] = type_id;
    output_words[insert_offset + 7] = filtered_id;
    output_words[insert_offset + 8] = condition_id;
    output_words[insert_offset + 9] = selected_id;
    output_words[insert_offset + 10] = fallback_id;
    memcpy(&output_words[insert_offset + 11], &input_words[insert_offset],
            (input_count - insert_offset) * sizeof(*output_words));
    output_words[3] = filtered_id + 1;

    for (offset = 5; offset < output_count;)
    {
        uint32_t instruction = output_words[offset];
        uint32_t instruction_words = instruction >> 16;
        uint32_t opcode = instruction & 0xffffu;

        if (!instruction_words || offset + instruction_words > output_count)
        {
            fprintf(stderr, "Malformed output instruction at word %zu\n", offset);
            free(output_words);
            free(input_words);
            return EXIT_FAILURE;
        }
        if (instruction_words == 5 && opcode == SPV_OP_I_EQUAL &&
                output_words[offset + 3] == extract_id &&
                output_words[offset + 4] == zero_id)
        {
            output_words[offset + 3] = filtered_id;
            ++zero_matches;
        }
        else if (instruction_words == 5 && opcode == SPV_OP_SHIFT_LEFT_LOGICAL &&
                output_words[offset + 3] == extract_id &&
                output_words[offset + 4] == shift_id)
        {
            output_words[offset + 3] = filtered_id;
            ++shift_matches;
        }
        offset += instruction_words;
    }

    if (zero_matches != 1 || shift_matches != 1)
    {
        fprintf(stderr, "Expected one zero test and one record-index shift; "
                "found %zu and %zu\n", zero_matches, shift_matches);
        free(output_words);
        free(input_words);
        return EXIT_FAILURE;
    }

    write_module(argv[2], output_words, output_count);
    printf("ISOLATE_ID_PATCH input=%s output=%s extracted_id=%"PRIu32
            " selected_constant_id=%"PRIu32" fallback_constant_id=%"PRIu32
            " condition_id=%"PRIu32" filtered_id=%"PRIu32"\n",
            argv[1], argv[2], extract_id, selected_id, fallback_id,
            condition_id, filtered_id);
    free(output_words);
    free(input_words);
    return EXIT_SUCCESS;
}
