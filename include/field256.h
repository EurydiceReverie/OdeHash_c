#ifndef FIELD256_H
#define FIELD256_H

#include <stdint.h>
#include <string.h>

/*
 * Finite field F_p  with  p = 2^255 - 19  (Curve25519 prime)
 *
 * Representation: 4 × 64-bit words, little-endian.
 *   element = w[0] + w[1]*2^64 + w[2]*2^128 + w[3]*2^192
 *
 * All arithmetic is branchless and constant-time.
 */

typedef struct { uint64_t w[4]; } fe256_t;

/* p = 2^255 - 19 */
static const fe256_t FE256_P = {{
    0xFFFFFFFFFFFFFFEDULL,
    0xFFFFFFFFFFFFFFFFULL,
    0xFFFFFFFFFFFFFFFFULL,
    0x7FFFFFFFFFFFFFFFULL
}};

/* 2*p for reduction */
static const fe256_t FE256_2P = {{
    0xFFFFFFFFFFFFFFD9ULL,
    0xFFFFFFFFFFFFFFFFULL,
    0xFFFFFFFFFFFFFFFFULL,
    0xFFFFFFFFFFFFFFFFULL  /* bit 255 set */
}};

static const fe256_t FE256_ZERO = {{0, 0, 0, 0}};
static const fe256_t FE256_ONE  = {{1, 0, 0, 0}};

/* ---- comparison ---- */

/* returns 0xFFFFFFFF.. if a >= b, else 0 */
static inline uint64_t ct_gte_mask(fe256_t a, fe256_t b)
{
    /* compare from high word to low */
    uint64_t gt = 0, eq = 0xFFFFFFFFFFFFFFFFULL;
    for (int i = 3; i >= 0; i--) {
        uint64_t a_gt = (a.w[i] > b.w[i]) ? 0xFFFFFFFFFFFFFFFFULL : 0;
        uint64_t a_eq = (a.w[i] == b.w[i]) ? 0xFFFFFFFFFFFFFFFFULL : 0;
        gt = gt | (eq & a_gt);
        eq = eq & a_eq;
    }
    return gt | eq;  /* >= */
}

/* ---- addition ---- */

static inline fe256_t fe256_add(fe256_t a, fe256_t b)
{
    fe256_t r;
    __uint128_t carry = 0;
    for (int i = 0; i < 4; i++) {
        carry += (__uint128_t)a.w[i] + b.w[i];
        r.w[i] = (uint64_t)carry;
        carry >>= 64;
    }
    /* reduce: if r >= p then r -= p (may need twice since a+b can be up to 2p-2) */
    for (int pass = 0; pass < 2; pass++) {
        uint64_t mask = ct_gte_mask(r, FE256_P);
        uint64_t borrow = 0;
        for (int i = 0; i < 4; i++) {
            uint64_t sub = FE256_P.w[i] & mask;
            uint64_t b_sum = sub + borrow;
            int b_overflow = (b_sum < sub) ? 1 : 0;
            borrow = (b_overflow || r.w[i] < b_sum) ? 1 : 0;
            r.w[i] = r.w[i] - b_sum;
        }
    }
    return r;
}

/* ---- subtraction ---- */

static inline fe256_t fe256_sub(fe256_t a, fe256_t b)
{
    fe256_t r;
    uint64_t borrow = 0;
    for (int i = 0; i < 4; i++) {
        uint64_t b_sum = b.w[i] + borrow;
        /* handle overflow in b.w[i] + borrow */
        int b_overflow = (b_sum < b.w[i]) ? 1 : 0;
        borrow = (b_overflow || a.w[i] < b_sum) ? 1 : 0;
        r.w[i] = a.w[i] - b_sum;
    }
    /* if underflow, add p */
    uint64_t mask = borrow ? 0xFFFFFFFFFFFFFFFFULL : 0;
    __uint128_t carry = 0;
    for (int i = 0; i < 4; i++) {
        carry += (__uint128_t)r.w[i] + (FE256_P.w[i] & mask);
        r.w[i] = (uint64_t)carry;
        carry >>= 64;
    }
    return r;
}

/* ---- multiplication ---- */

/*
 * Schoolbook 4×4 → 8 words, then reduce mod 2^255 - 19.
 *
 * Reduction: 2^256 ≡ 38 (mod p)
 *   N = low + high * 2^256 ≡ low + 38 * high (mod p)
 * where low = p[0..3], high = p[4..7].
 */
static inline fe256_t fe256_mul(fe256_t a, fe256_t b)
{
    /* 8-word product */
    uint64_t prod[8] = {0};
    __uint128_t carry;

    for (int i = 0; i < 4; i++) {
        carry = 0;
        for (int j = 0; j < 4; j++) {
            __uint128_t tmp = (__uint128_t)a.w[i] * b.w[j] + prod[i + j] + (uint64_t)carry;
            prod[i + j] = (uint64_t)tmp;
            carry = tmp >> 64;
        }
        prod[i + 4] += (uint64_t)carry;  /* ADD, not overwrite */
    }

    /* reduce: result = prod[0..3] + 38 * prod[4..7] */
    __uint128_t c = 0;
    uint64_t r[4];
    for (int i = 0; i < 4; i++) {
        c += (__uint128_t)prod[i] + (__uint128_t)38 * prod[i + 4];
        r[i] = (uint64_t)c;
        c >>= 64;
    }
    /* fold remaining carry: c * 2^256 ≡ c * 38 (mod p)
     * c is at most ~39 after the reduction, so one unconditional pass suffices.
     */
    {
        __uint128_t fold = (__uint128_t)(uint64_t)c * 38;
        for (int i = 0; i < 4; i++) {
            fold += r[i];
            r[i] = (uint64_t)fold;
            fold >>= 64;
        }
    }

    fe256_t result = {{r[0], r[1], r[2], r[3]}};

    /* final reduction — unconditional 4 passes (result < 40p, needs ~40 subtractions,
     * but each pass uses branchless conditional subtraction via mask) */
    for (int pass = 0; pass < 40; pass++) {
        uint64_t mask = ct_gte_mask(result, FE256_P);
        uint64_t borrow = 0;
        for (int i = 0; i < 4; i++) {
            uint64_t sub = FE256_P.w[i] & mask;
            uint64_t b_sum = sub + borrow;
            int b_overflow = (b_sum < sub) ? 1 : 0;
            borrow = (b_overflow || result.w[i] < b_sum) ? 1 : 0;
            result.w[i] = result.w[i] - b_sum;
        }
    }
    return result;
}

static inline fe256_t fe256_sqr(fe256_t a) { return fe256_mul(a, a); }

/* ---- inversion: a^(p-2) mod p ---- */

static inline fe256_t fe256_inv(fe256_t a)
{
    /* p - 2 = 2^255 - 21 */
    fe256_t result = FE256_ONE;
    fe256_t base = a;

    /* exponent has 255 bits; bit 254..0 */
    /* p-2 in words: {0xFFFFFFFFFFFFFFEB, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0x7FFFFFFFFFFFFFFF} */
    uint64_t exp[4] = {0xFFFFFFFFFFFFFFEBULL, 0xFFFFFFFFFFFFFFFFULL,
                       0xFFFFFFFFFFFFFFFFULL, 0x7FFFFFFFFFFFFFFFULL};

    for (int word = 0; word < 4; word++) {
        uint64_t e = exp[word];
        int bits = (word < 3) ? 64 : 63;  /* top word has 63 bits */
        for (int bit = 0; bit < bits; bit++) {
            /* branchless: if (e & 1) result = result * base */
            uint64_t mask = -(e & 1);
            fe256_t prod = fe256_mul(result, base);
            for (int i = 0; i < 4; i++)
                result.w[i] = (prod.w[i] & mask) | (result.w[i] & ~mask);
            base = fe256_sqr(base);
            e >>= 1;
        }
    }
    return result;
}

/* ---- conversion ---- */

static inline fe256_t fe256_from_u64(uint64_t v)
{
    fe256_t r = {{v, 0, 0, 0}};
    return r;
}

static inline void fe256_to_bytes(uint8_t out[32], fe256_t a)
{
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 8; j++)
            out[i * 8 + j] = (uint8_t)(a.w[i] >> (8 * j));
    }
}

static inline fe256_t fe256_from_bytes(const uint8_t in[32])
{
    fe256_t r;
    for (int i = 0; i < 4; i++) {
        r.w[i] = 0;
        for (int j = 0; j < 8; j++)
            r.w[i] |= (uint64_t)in[i * 8 + j] << (8 * j);
    }
    /* mask top bit (Curve25519 convention) */
    r.w[3] &= 0x7FFFFFFFFFFFFFFFULL;
    return r;
}

/* ---- secure wipe ---- */

static inline void fe256_wipe(fe256_t *a)
{
    volatile uint8_t *p = (volatile uint8_t *)a;
    for (size_t i = 0; i < sizeof(fe256_t); i++) p[i] = 0;
}

#endif /* FIELD256_H */
