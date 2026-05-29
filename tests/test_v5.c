#include "ode_hash_v5.h"
#include <stdio.h>
#include <string.h>

static void print_hash(const uint8_t h[32])
{
    for (int i = 0; i < 32; i++) printf("%02x", h[i]);
}

int main(void)
{
    printf("=== ODE-Hash v5 (256-bit field) Test Vectors ===\n\n");

    struct { const char *input; } tests[] = {
        {""}, {"a"}, {"abc"}, {"hello"}, {"Hello, World!"},
    };
    int n = sizeof(tests) / sizeof(tests[0]);
    int passed = 0;

    for (int i = 0; i < n; i++) {
        uint8_t h[32];
        ode_hash_v5((const uint8_t *)tests[i].input, strlen(tests[i].input), h);
        printf("  \"%s\"\n    ", tests[i].input);
        print_hash(h);
        int nonzero = 0;
        for (int j = 0; j < 32; j++) if (h[j]) { nonzero = 1; break; }
        printf("  %s\n", nonzero ? "PASS" : "FAIL");
        if (nonzero) passed++;
    }

    /* determinism */
    uint8_t h1[32], h2[32];
    ode_hash_v5((const uint8_t *)"test", 4, h1);
    ode_hash_v5((const uint8_t *)"test", 4, h2);
    int det = (memcmp(h1, h2, 32) == 0);
    printf("  Determinism: %s\n", det ? "PASS" : "FAIL");
    if (det) passed++;

    /* different */
    ode_hash_v5((const uint8_t *)"aaa", 3, h1);
    ode_hash_v5((const uint8_t *)"bbb", 3, h2);
    int diff = (memcmp(h1, h2, 32) != 0);
    printf("  Different:   %s\n", diff ? "PASS" : "FAIL");
    if (diff) passed++;

    printf("\n  %d/%d passed\n", passed, n + 2);
    return passed == n + 2 ? 0 : 1;
}
