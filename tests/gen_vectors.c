#include "ode_hash_v5.h"
#include <stdio.h>
#include <string.h>

int main(void)
{
    struct { const char *input; } tests[] = {
        {""}, {"a"}, {"abc"}, {"hello"}, {"Hello, World!"},
        {"The quick brown fox jumps over the lazy dog"},
        {"0123456789abcdef"},
        {"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"},
        {"x"}, {"test123"},
    };
    int n = sizeof(tests) / sizeof(tests[0]);

    for (int i = 0; i < n; i++) {
        uint8_t h[32];
        ode_hash_v5((const uint8_t *)tests[i].input, strlen(tests[i].input), h);
        printf("  \"%s\"\n    ", tests[i].input);
        for (int j = 0; j < 32; j++) printf("%02x", h[j]);
        printf("\n");
    }
    return 0;
}
