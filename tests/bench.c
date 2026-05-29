/*
 * Benchmark for ODE-Hash v4
 * Measures throughput in bytes/second and cycles/byte.
 */

#include "ode_hash_v3.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static double bench_size(size_t size, int iterations)
{
    uint8_t *input = (uint8_t *)malloc(size);
    uint8_t output[32];
    for (size_t i = 0; i < size; i++)
        input[i] = (uint8_t)(rand() & 0xFF);

    clock_t start = clock();
    for (int i = 0; i < iterations; i++)
        ode_hash_v3(input, size, output);
    clock_t end = clock();

    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
    double bytes_per_sec = (double)size * iterations / elapsed;

    free(input);
    return bytes_per_sec;
}

int main(void)
{
    printf("=== ODE-Hash v4 Benchmark ===\n\n");

    struct { size_t size; int iters; const char *label; } tests[] = {
        {     32,  100, "    32 B" },
        {     64,  100, "    64 B" },
        {    128,   50, "   128 B" },
        {   1024,   20, "  1024 B" },
        {   4096,   10, "  4096 B" },
        {  65536,    2, " 64 KiB" },
    };
    int ntests = sizeof(tests) / sizeof(tests[0]);

    printf("  %-10s  %5s  %12s  %10s\n", "Size", "Iters", "Bytes/sec", "ms/hash");
    printf("  %-10s  %5s  %12s  %10s\n", "----", "-----", "---------", "-------");

    for (int t = 0; t < ntests; t++) {
        uint8_t *input = (uint8_t *)malloc(tests[t].size);
        uint8_t output[32];
        for (size_t i = 0; i < tests[t].size; i++)
            input[i] = (uint8_t)(rand() & 0xFF);

        clock_t start = clock();
        for (int i = 0; i < tests[t].iters; i++)
            ode_hash_v3(input, tests[t].size, output);
        clock_t end = clock();

        double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
        double bytes_per_sec = (double)tests[t].size * tests[t].iters / elapsed;
        double ms_per_hash = elapsed / tests[t].iters * 1000.0;

        printf("  %-10s  %5d  %12.0f  %10.3f\n",
               tests[t].label, tests[t].iters, bytes_per_sec, ms_per_hash);

        free(input);
    }

    printf("\n");
    return 0;
}
