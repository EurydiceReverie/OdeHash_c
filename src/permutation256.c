#include "permutation256.h"
#include <string.h>

static fe256_t rc256(int round, int index)
{
    /* Round constants: nothing-up-my-sleeve numbers (first 8 words of SHA-256 IV).
 * Used only to break symmetry — OdeHash does NOT use SHA-256. */
    static const uint64_t BASE[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19,
    };
    return fe256_from_u64(BASE[index] + (uint64_t)round);
}

void permute256(fe256_t state[P256_STATE_SIZE])
{
    fe256_t old[P256_STATE_SIZE];
    memcpy(old, state, sizeof(fe256_t) * P256_STATE_SIZE);

    for (int round = 0; round < P256_ROUNDS; round++) {
        /* 1. round constants */
        for (int i = 0; i < 8; i++)
            state[i] = fe256_add(state[i], rc256(round, i));

        /* 2. polynomial coefficients from all 16 state words */
        fe256_t p1[8], q1[8], r1[8], c1[8];
        fe256_t p2[8], q2[8], r2[8], c2[8];

        for (int i = 0; i < 8; i++) {
            fe256_t s = state[i];
            fe256_t t = state[(i + 8) % 16];

            p1[i] = fe256_add(s, fe256_mul(t, fe256_from_u64(0x9e3779b9)));
            q1[i] = fe256_mul(fe256_add(s, t), fe256_from_u64(0x517cc1b7));
            r1[i] = fe256_sqr(fe256_add(s, fe256_mul(t, fe256_from_u64(0x85ebca6b))));
            c1[i] = fe256_mul(s, fe256_add(t, fe256_from_u64(0xc2b2ae35)));

            p2[i] = fe256_mul(fe256_add(s, t), fe256_from_u64(0x27d4eb2f));
            q2[i] = fe256_mul(s, fe256_add(t, fe256_from_u64(0x165667b1)));
            r2[i] = fe256_sqr(fe256_add(fe256_mul(s, fe256_from_u64(0x9b05688c)), t));
            c2[i] = fe256_add(fe256_mul(s, t), fe256_from_u64(0xa54ff53a));
        }

        /* 3. initial conditions */
        fe256_t a[P256_STEPS + 1], b[P256_STEPS + 1];
        a[0] = fe256_add(state[8], fe256_mul(state[10], state[11]));
        b[0] = fe256_add(state[9], fe256_mul(state[12], state[13]));
        fe256_t mix_a = state[14];
        fe256_t mix_b = state[15];

        /* pre-compute inverses 1..16 */
        fe256_t inv[P256_STEPS + 1];
        inv[0] = FE256_ZERO;
        for (int i = 1; i <= P256_STEPS; i++)
            inv[i] = fe256_inv(fe256_from_u64(i));

        /* 4. Taylor recurrence */
        for (int n = 0; n < P256_STEPS; n++) {
            int kmax = n < 7 ? n : 7;

            fe256_t sum_a = p1[n % 8];
            for (int k = 0; k <= kmax; k++) {
                sum_a = fe256_add(sum_a, fe256_mul(q1[k], a[n - k]));
                fe256_t inner_r = FE256_ZERO;
                for (int j = 0; j <= n - k; j++)
                    inner_r = fe256_add(inner_r, fe256_mul(a[j], a[n - k - j]));
                sum_a = fe256_add(sum_a, fe256_mul(r1[k], inner_r));
                fe256_t inner_c = FE256_ZERO;
                for (int j = 0; j <= n - k; j++)
                    inner_c = fe256_add(inner_c, fe256_mul(a[j], b[n - k - j]));
                sum_a = fe256_add(sum_a, fe256_mul(c1[k], inner_c));
            }
            sum_a = fe256_add(sum_a, fe256_mul(mix_a, fe256_from_u64(n + 1)));
            a[n + 1] = fe256_mul(sum_a, inv[n + 1]);

            fe256_t sum_b = p2[n % 8];
            for (int k = 0; k <= kmax; k++) {
                sum_b = fe256_add(sum_b, fe256_mul(q2[k], b[n - k]));
                fe256_t inner_r = FE256_ZERO;
                for (int j = 0; j <= n - k; j++)
                    inner_r = fe256_add(inner_r, fe256_mul(b[j], b[n - k - j]));
                sum_b = fe256_add(sum_b, fe256_mul(r2[k], inner_r));
                fe256_t inner_c = FE256_ZERO;
                for (int j = 0; j <= n - k; j++)
                    inner_c = fe256_add(inner_c, fe256_mul(a[j], b[n - k - j]));
                sum_b = fe256_add(sum_b, fe256_mul(c2[k], inner_c));
            }
            sum_b = fe256_add(sum_b, fe256_mul(mix_b, fe256_from_u64(n + 1)));
            b[n + 1] = fe256_mul(sum_b, inv[n + 1]);
        }

        /* 5. Feistel feedback */
        for (int i = 0; i < 8; i++) {
            state[i]     = fe256_add(a[9 + i], old[i]);
            state[8 + i] = fe256_add(b[9 + i], old[8 + i]);
        }

        memcpy(old, state, sizeof(fe256_t) * P256_STATE_SIZE);

        /* wipe */
        memset(a, 0, sizeof(a));
        memset(b, 0, sizeof(b));
    }

    memset(old, 0, sizeof(old));
}

void absorb256(fe256_t state[P256_STATE_SIZE], const fe256_t block[P256_RATE])
{
    for (int i = 0; i < P256_RATE; i++)
        state[i] = fe256_add(state[i], block[i]);
    permute256(state);
}
