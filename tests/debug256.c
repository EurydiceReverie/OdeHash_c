#include "field256.h"
#include "permutation256.h"
#include <stdio.h>

void p256(const char *label, fe256_t a)
{
    printf("  %s = %016llx %016llx %016llx %016llx\n", label,
        (unsigned long long)a.w[3], (unsigned long long)a.w[2],
        (unsigned long long)a.w[1], (unsigned long long)a.w[0]);
}

int main(void)
{
    printf("=== C v5 Debug ===\n\n");

    /* test field: 2 * 3 = 6 */
    fe256_t two = fe256_from_u64(2);
    fe256_t three = fe256_from_u64(3);
    fe256_t six = fe256_mul(two, three);
    p256("2*3", six);

    /* test field: inv(2) */
    fe256_t inv2 = fe256_inv(two);
    fe256_t check = fe256_mul(two, inv2);
    p256("2*inv(2)", check);

    /* IV words (same as sponge256.c) */
    static const uint64_t IV[16] = {
        0x6a09e667f3bcc908ULL, 0xbb67ae8584caa73bULL,
        0x3c6ef372fe94f82bULL, 0xa54ff53a5f1d36f1ULL,
        0x510e527fade682d1ULL, 0x9b05688c2b3e6c1fULL,
        0x1f83d9abfb41bd6bULL, 0x5be0cd19137e2179ULL,
        0xcbbb9d5d66a37e35ULL, 0x629a292a366cd219ULL,
        0x9159015a1e78b8f2ULL, 0x152fecd8f70e5939ULL,
        0x67332667ffc00b31ULL, 0x8eb44a8768581511ULL,
        0xdb0c2e0d64f98fa7ULL, 0x47b5481dbefa4fa4ULL,
    };

    /* test permutation: init state, run one perm */
    fe256_t state[P256_STATE_SIZE];
    for (int i = 0; i < P256_STATE_SIZE; i++)
        state[i] = fe256_from_u64(IV[i]);

    printf("\nState before permute:\n");
    for (int i = 0; i < 4; i++) {
        char label[32];
        snprintf(label, sizeof(label), "state[%d]", i);
        p256(label, state[i]);
    }

    permute256(state);

    printf("\nState after permute:\n");
    for (int i = 0; i < 4; i++) {
        char label[32];
        snprintf(label, sizeof(label), "state[%d]", i);
        p256(label, state[i]);
    }

    return 0;
}
