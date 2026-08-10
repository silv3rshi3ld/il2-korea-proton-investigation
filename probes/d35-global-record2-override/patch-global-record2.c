#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SPIRV_MAGIC 0x07230203u
#define SPV_OP_COMPOSITE_CONSTRUCT 80u
#define SPV_OP_IMAGE_FETCH 95u
#define SPV_OP_F_MUL 133u
#define SPV_OP_SELECT 169u

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
    uint32_t vector_type, grid_result, grid_image, grid_coordinate;
    uint32_t uint_zero, uint_one, scalar_type, selected_id;
    uint32_t select_result, select_condition, select_true, select_false;
    uint32_t float_type, multiply_result, float_one, light_factor;
    uint32_t *words;
    size_t word_count, offset;
    size_t grid_matches = 0, select_matches = 0, visibility_matches = 0;

    if (argc != 19)
    {
        fprintf(stderr, "Usage: %s INPUT.spv OUTPUT.spv VECTOR_TYPE "
                "GRID_RESULT GRID_IMAGE GRID_COORD UINT_ZERO UINT_ONE "
                "SCALAR_TYPE SELECT_RESULT SELECT_CONDITION SELECTED_ID "
                "SELECT_FALLBACK FLOAT_TYPE MULTIPLY_RESULT FLOAT_ONE "
                "LIGHT_FACTOR\n", argv[0]);
        return EXIT_FAILURE;
    }

    vector_type = parse_id(argv[3], "grid vector type ID");
    grid_result = parse_id(argv[4], "grid result ID");
    grid_image = parse_id(argv[5], "grid image ID");
    grid_coordinate = parse_id(argv[6], "grid coordinate ID");
    uint_zero = parse_id(argv[7], "uint-zero ID");
    uint_one = parse_id(argv[8], "uint-one ID");
    scalar_type = parse_id(argv[9], "scalar uint type ID");
    select_result = parse_id(argv[10], "D32 selected ID result");
    select_condition = parse_id(argv[11], "D32 selected ID condition");
    selected_id = parse_id(argv[12], "record-2 constant ID");
    select_false = parse_id(argv[13], "D32 fallback ID");
    float_type = parse_id(argv[14], "float type ID");
    multiply_result = parse_id(argv[15], "D34 visibility multiply result");
    float_one = parse_id(argv[16], "float-one ID");
    light_factor = parse_id(argv[17], "ordinary light factor ID");

    /* argv[18] is intentionally reserved for a contract version. */
    if (parse_id(argv[18], "contract version") != 1)
    {
        fprintf(stderr, "Unsupported contract version\n");
        return EXIT_FAILURE;
    }

    select_true = selected_id;
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

        if (instruction_words == 7 && opcode == SPV_OP_IMAGE_FETCH &&
                words[offset + 1] == vector_type &&
                words[offset + 2] == grid_result &&
                words[offset + 3] == grid_image &&
                words[offset + 4] == grid_coordinate &&
                words[offset + 5] == 2 && words[offset + 6] == uint_zero)
        {
            /* Packed light-list word: start=0, count=1. Keep the same result
             * type, result ID, instruction length, and zero constants. */
            words[offset] = (7u << 16) | SPV_OP_COMPOSITE_CONSTRUCT;
            words[offset + 3] = uint_one;
            words[offset + 4] = uint_zero;
            words[offset + 5] = uint_zero;
            words[offset + 6] = uint_zero;
            ++grid_matches;
        }
        else if (instruction_words == 6 && opcode == SPV_OP_SELECT &&
                words[offset + 1] == scalar_type &&
                words[offset + 2] == select_result &&
                words[offset + 3] == select_condition &&
                words[offset + 4] == select_true &&
                words[offset + 5] == select_false)
        {
            /* Preserve the D32 instruction shape but make both outcomes ID 2.
             * The t10 fetch remains present, while membership no longer
             * chooses the record. */
            words[offset + 5] = selected_id;
            ++select_matches;
        }
        else if (instruction_words == 5 && opcode == SPV_OP_F_MUL &&
                words[offset + 1] == float_type &&
                words[offset + 2] == multiply_result &&
                words[offset + 3] == float_one &&
                words[offset + 4] == light_factor)
        {
            /* Require the accepted D34 fully-visible shadow control. */
            ++visibility_matches;
        }

        offset += instruction_words;
    }

    if (grid_matches != 1 || select_matches != 1 || visibility_matches != 1)
    {
        fprintf(stderr, "Input contract mismatch: grid=%zu select=%zu "
                "d34_visibility=%zu\n", grid_matches, select_matches,
                visibility_matches);
        free(words);
        return EXIT_FAILURE;
    }

    write_module(argv[2], words, word_count);
    printf("GLOBAL_RECORD2_PATCH input=%s output=%s packed_start=0 "
            "packed_count=1 selected_id=%"PRIu32" shadow_visibility=1\n",
            argv[1], argv[2], selected_id);
    free(words);
    return EXIT_SUCCESS;
}
