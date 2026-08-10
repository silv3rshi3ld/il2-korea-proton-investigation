#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SPIRV_MAGIC 0x07230203u
#define SPV_OP_COMPOSITE_EXTRACT 81u
#define SPV_OP_BITWISE_OR 197u

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
    uint32_t result_type_id, result_id, composite_id, one_id, zero_id;
    size_t module_words, offset, matches = 0;
    uint32_t *words;

    if (argc != 8)
    {
        fprintf(stderr, "Usage: %s INPUT.spv OUTPUT.spv TYPE_ID RESULT_ID "
                "COMPOSITE_ID ONE_ID ZERO_ID\n", argv[0]);
        return EXIT_FAILURE;
    }

    result_type_id = parse_id(argv[3], "result type ID");
    result_id = parse_id(argv[4], "result ID");
    composite_id = parse_id(argv[5], "composite ID");
    one_id = parse_id(argv[6], "one constant ID");
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

        if (instruction_words == 5 && opcode == SPV_OP_COMPOSITE_EXTRACT &&
                words[offset + 1] == result_type_id &&
                words[offset + 2] == result_id &&
                words[offset + 3] == composite_id && words[offset + 4] == 0)
        {
            /* Preserve the real loop count and complete per-light evaluation,
             * but substitute the first valid non-sentinel light ID. */
            words[offset] = (5u << 16) | SPV_OP_BITWISE_OR;
            words[offset + 3] = one_id;
            words[offset + 4] = zero_id;
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
    printf("D27_PATCH input=%s output=%s result_id=%"PRIu32
            " replacement=light_id_1 matches=%zu\n",
            argv[1], argv[2], result_id, matches);
    free(words);
    return EXIT_SUCCESS;
}
