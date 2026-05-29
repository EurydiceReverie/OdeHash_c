#include "ode_hash_v5.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHUNK_SIZE 65536

static void usage(const char *prog)
{
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  %s <string>\n", prog);
    fprintf(stderr, "  %s -f <file> [-p] [--format]\n", prog);
}

static void print_progress(size_t current, size_t total)
{
    int pct = (total > 0) ? (int)((unsigned long long)current * 100 / total) : 0;
    int bars = pct / 2;
    fprintf(stderr, "\rHashing: %3d%% [", pct);
    for (int i = 0; i < 50; i++)
        fprintf(stderr, "%c", i < bars ? '=' : (i == bars ? '>' : ' '));
    fprintf(stderr, "] %llu/%llu bytes",
            (unsigned long long)current, (unsigned long long)total);
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        usage(argv[0]);
        return 1;
    }

    const char *filename = NULL;
    const char *input_str = NULL;
    int show_format = 0;
    int show_progress = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-f") == 0 && i + 1 < argc) {
            filename = argv[++i];
        } else if (strcmp(argv[i], "--format") == 0) {
            show_format = 1;
        } else if (strcmp(argv[i], "-p") == 0) {
            show_progress = 1;
        } else if (input_str == NULL && filename == NULL) {
            input_str = argv[i];
        }
    }

    /* string mode */
    if (input_str != NULL && filename == NULL) {
        uint8_t hash[ODE_HASH5_SIZE];
        ode_hash_v5((const uint8_t *)input_str, strlen(input_str), hash);
        for (int i = 0; i < ODE_HASH5_SIZE; i++)
            printf("%02x", hash[i]);
        printf("\n");
        return 0;
    }

    /* file mode */
    if (filename == NULL) {
        usage(argv[0]);
        return 1;
    }

    FILE *f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "Error: cannot open '%s'\n", filename);
        return 1;
    }

    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (file_size < 0) {
        fprintf(stderr, "Error: cannot determine file size\n");
        fclose(f);
        return 1;
    }

    /* streaming hash */
    uint8_t *chunk = (uint8_t *)malloc(CHUNK_SIZE);
    if (!chunk) {
        fprintf(stderr, "Error: malloc failed\n");
        fclose(f);
        return 1;
    }

    /* use single-shot for now (v5 is slow, files are small) */
    uint8_t *file_data = (uint8_t *)malloc((size_t)file_size);
    if (!file_data) {
        fprintf(stderr, "Error: malloc failed\n");
        free(chunk);
        fclose(f);
        return 1;
    }
    fread(file_data, 1, (size_t)file_size, f);
    fclose(f);

    uint8_t hash[ODE_HASH5_SIZE];
    ode_hash_v5(file_data, (size_t)file_size, hash);

    if (show_format) {
        for (int i = 0; i < ODE_HASH5_SIZE; i++)
            printf("%02x", hash[i]);
        printf("  %s\n", filename);
    } else {
        for (int i = 0; i < ODE_HASH5_SIZE; i++)
            printf("%02x", hash[i]);
        printf("\n");
    }

    free(file_data);
    free(chunk);
    return 0;
}
