#include "ode_hash_v5.h"
#include <stdio.h>

int main(void)
{
    FILE *f = fopen("dieharder_stream_v5.bin", "wb");
    if (!f) { fprintf(stderr, "Cannot open output\n"); return 1; }

    printf("Generating 10 MB v5 Dieharder stream...\n");

    uint8_t hash[32];
    size_t total = 0;
    size_t target = 64 * 1024;  /* 64 KB — v5 is slow */

    for (uint32_t i = 0; total < target; i++) {
        uint8_t input[4];
        input[0] = (i >> 24) & 0xFF;
        input[1] = (i >> 16) & 0xFF;
        input[2] = (i >>  8) & 0xFF;
        input[3] =  i        & 0xFF;

        ode_hash_v5(input, 4, hash);
        fwrite(hash, 1, 32, f);
        total += 32;

        if ((i + 1) % 1000 == 0)
            printf("\r  %zu / %zu bytes", total, target);
    }

    fclose(f);
    printf("\n  Wrote %zu bytes to dieharder_stream_v5.bin\n", total);
    return 0;
}
