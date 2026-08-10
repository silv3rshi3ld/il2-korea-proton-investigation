#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SPIRV_MAGIC 0x07230203u
#define SPV_OP_CONSTANT 43u
#define SPV_OP_IMAGE_SAMPLE_DREF_EXPLICIT_LOD 90u
#define SPV_OP_F_MUL 133u
#define SPV_OP_SELECT 169u
#define SPV_OP_I_EQUAL 170u

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

int main(int argc, char **argv)
{
    uint32_t float_type, bool_type, uint_type;
    uint32_t extract_id, selected_constant, fallback_constant;
    uint32_t condition_id, filtered_id, one_id;
    uint32_t multiply_result, shadow_factor, light_factor;
    uint32_t *words;
    size_t word_count, offset, patch_offset = 0;
    size_t condition_matches = 0, select_matches = 0, one_matches = 0;
    size_t multiply_matches = 0, dref_samples = 0;

    if (argc != 15)
    {
        fprintf(stderr, "Usage: %s INPUT.spv OUTPUT.spv FLOAT_TYPE BOOL_TYPE "
                "UINT_TYPE EXTRACT_ID SELECTED_CONSTANT FALLBACK_CONSTANT "
                "CONDITION_ID FILTERED_ID ONE_ID MULTIPLY_RESULT "
                "SHADOW_FACTOR LIGHT_FACTOR\n", argv[0]);
        return EXIT_FAILURE;
    }

    float_type = parse_id(argv[3], "float type ID");
    bool_type = parse_id(argv[4], "boolean type ID");
    uint_type = parse_id(argv[5], "uint type ID");
    extract_id = parse_id(argv[6], "extracted light ID");
    selected_constant = parse_id(argv[7], "selected light constant ID");
    fallback_constant = parse_id(argv[8], "fallback light constant ID");
    condition_id = parse_id(argv[9], "D32 condition ID");
    filtered_id = parse_id(argv[10], "D32 filtered ID");
    one_id = parse_id(argv[11], "float-one constant ID");
    multiply_result = parse_id(argv[12], "visibility multiply result ID");
    shadow_factor = parse_id(argv[13], "shadow factor ID");
    light_factor = parse_id(argv[14], "ordinary light factor ID");

    words = read_module(argv[1], &word_count);

    for (offset = 5; offset < word_count;)
    {
        uint32_t instruction = words[offset];
        uint32_t instruction_words = instruction >> 16;
        uint32_t opcode = instruction & 0xffffu;

        if (!instruction_words || offset + instruction_words > word_count)
        {
            fprintf(stderr, "Malformed instruction at word %zu\n", offset);
            free(words);
            return EXIT_FAILURE;
        }

        if (instruction_words == 5 && opcode == SPV_OP_I_EQUAL &&
                words[offset + 1] == bool_type &&
                words[offset + 2] == condition_id &&
                words[offset + 3] == extract_id &&
                words[offset + 4] == selected_constant)
            ++condition_matches;
        else if (instruction_words == 6 && opcode == SPV_OP_SELECT &&
                words[offset + 1] == uint_type &&
                words[offset + 2] == filtered_id &&
                words[offset + 3] == condition_id &&
                words[offset + 4] == selected_constant &&
                words[offset + 5] == fallback_constant)
            ++select_matches;
        else if (instruction_words == 4 && opcode == SPV_OP_CONSTANT &&
                words[offset + 1] == float_type &&
                words[offset + 2] == one_id &&
                words[offset + 3] == UINT32_C(0x3f800000))
            ++one_matches;
        else if (instruction_words == 5 && opcode == SPV_OP_F_MUL &&
                words[offset + 1] == float_type &&
                words[offset + 2] == multiply_result &&
                words[offset + 3] == shadow_factor &&
                words[offset + 4] == light_factor)
        {
            patch_offset = offset + 3;
            ++multiply_matches;
        }

        if (opcode == SPV_OP_IMAGE_SAMPLE_DREF_EXPLICIT_LOD)
            ++dref_samples;

        offset += instruction_words;
    }

    if (condition_matches != 1 || select_matches != 1 || one_matches != 1 ||
            multiply_matches != 1 || dref_samples != 7)
    {
        fprintf(stderr, "Input contract mismatch: condition=%zu select=%zu "
                "float_one=%zu visibility_multiply=%zu dref_samples=%zu\n",
                condition_matches, select_matches, one_matches,
                multiply_matches, dref_samples);
        free(words);
        return EXIT_FAILURE;
    }

    words[patch_offset] = one_id;
    write_module(argv[2], words, word_count);
    printf("SHADOW_VISIBILITY_PATCH input=%s output=%s light_id=%"PRIu32
            " shadow_operand=%"PRIu32" replacement=%"PRIu32
            " multiply_result=%"PRIu32" dref_samples=%zu\n",
            argv[1], argv[2], selected_constant, shadow_factor, one_id,
            multiply_result, dref_samples);
    free(words);
    return EXIT_SUCCESS;
}
