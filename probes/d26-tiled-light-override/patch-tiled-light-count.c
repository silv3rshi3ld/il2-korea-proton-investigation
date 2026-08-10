#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SPIRV_MAGIC 0x07230203u
#define SPV_OP_COMPOSITE_CONSTRUCT 80u
#define SPV_OP_IMAGE_FETCH 95u

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
    uint32_t result_type_id, result_id, image_id, coordinate_id, zero_id;
    size_t module_words, offset, matches = 0;
    uint32_t *words;

    if (argc != 8)
    {
        fprintf(stderr, "Usage: %s INPUT.spv OUTPUT.spv TYPE_ID RESULT_ID "
                "IMAGE_ID COORDINATE_ID ZERO_ID\n", argv[0]);
        return EXIT_FAILURE;
    }

    result_type_id = parse_id(argv[3], "result type ID");
    result_id = parse_id(argv[4], "result ID");
    image_id = parse_id(argv[5], "image ID");
    coordinate_id = parse_id(argv[6], "coordinate ID");
    zero_id = parse_id(argv[7], "zero constant ID");
    words = read_module(argv[1], &module_words);

    for (offset = 5; offset < module_words;)
    {
        uint32_t instruction = words[offset];
        uint32_t instruction_words = instruction >> 16;
        uint32_t opcode = instruction & 0xffffu;

        if (!instruction_words || offset + instruction_words > module_words)
        {
            fprintf(stderr, "Malformed instruction at word %zu\n", offset);
            free(words);
            return EXIT_FAILURE;
        }

        if (instruction_words == 7 && opcode == SPV_OP_IMAGE_FETCH &&
                words[offset + 1] == result_type_id &&
                words[offset + 2] == result_id &&
                words[offset + 3] == image_id &&
                words[offset + 4] == coordinate_id &&
                words[offset + 5] == 2 && words[offset + 6] == zero_id)
        {
            /* Keep the instruction length, result type, and result ID exactly
             * unchanged. Replace the tiled-grid fetch with uint4(0), making
             * the shader take its existing zero-light branch. */
            words[offset] = (7u << 16) | SPV_OP_COMPOSITE_CONSTRUCT;
            words[offset + 3] = zero_id;
            words[offset + 4] = zero_id;
            words[offset + 5] = zero_id;
            words[offset + 6] = zero_id;
            ++matches;
        }

        offset += instruction_words;
    }

    if (matches != 1)
    {
        fprintf(stderr, "Expected exactly one target instruction in %s, "
                "found %zu\n", argv[1], matches);
        free(words);
        return EXIT_FAILURE;
    }

    write_module(argv[2], words, module_words);
    printf("D26_PATCH input=%s output=%s result_id=%"PRIu32
            " replacement=uint4_zero matches=%zu\n",
            argv[1], argv[2], result_id, matches);
    free(words);
    return EXIT_SUCCESS;
}
