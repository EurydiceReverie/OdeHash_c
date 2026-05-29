#include "sponge256.h"
#include <string.h>

#define ROT32(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

static void arx_quarter(uint32_t *v, int a, int b, int c, int d)
{
    v[a] += v[b]; v[d] ^= v[a]; v[d] = ROT32(v[d], 16);
    v[c] += v[d]; v[b] ^= v[c]; v[b] = ROT32(v[b], 12);
    v[a] += v[b]; v[d] ^= v[a]; v[d] = ROT32(v[d],  8);
    v[c] += v[d]; v[b] ^= v[c]; v[b] = ROT32(v[b],  7);
}

static void arx_16_rounds(uint32_t v[16])
{
    static const uint32_t RC[4] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
    };
    v[0] ^= RC[0]; v[1] ^= RC[1]; v[2] ^= RC[2]; v[3] ^= RC[3];

    for (uint32_t r = 0; r < 12; r++) {
        v[12] ^= r;
        arx_quarter(v, 0, 4,  8, 12);
        arx_quarter(v, 1, 5,  9, 13);
        arx_quarter(v, 2, 6, 10, 14);
        arx_quarter(v, 3, 7, 11, 15);
        arx_quarter(v, 0, 5, 10, 15);
        arx_quarter(v, 1, 6, 11, 12);
        arx_quarter(v, 2, 7,  8, 13);
        arx_quarter(v, 3, 4,  9, 14);
    }
}

/*
 * ARX finalization — uses ALL 256 bits from each field element.
 *
 * Strategy: 4 passes, each using a different 32-bit word from each element.
 *   Pass 0: state[i].w[0] (bits 0-31)
 *   Pass 1: state[i].w[1] (bits 32-63)
 *   Pass 2: state[i].w[2] (bits 64-95)
 *   Pass 3: state[i].w[3] (bits 96-127)  — only 15 elements, + round index
 *
 * Each pass produces 16 words.  XOR all four passes together.
 */
void arx_finalize256(const fe256_t state[16], uint8_t out[32])
{
    uint32_t accum[16] = {0};

    for (int pass = 0; pass < 4; pass++) {
        uint32_t v[16];
        for (int i = 0; i < 16; i++)
            v[i] = (uint32_t)(state[i].w[pass]);

        /* inject round index to differentiate passes */
        v[15] ^= (uint32_t)pass;

        arx_16_rounds(v);

        /* XOR into accumulator */
        for (int i = 0; i < 16; i++)
            accum[i] ^= v[i];
    }

    /* output first 8 words (256 bits) */
    for (int i = 0; i < 8; i++) {
        out[4*i]     = (uint8_t)(accum[i]);
        out[4*i + 1] = (uint8_t)(accum[i] >> 8);
        out[4*i + 2] = (uint8_t)(accum[i] >> 16);
        out[4*i + 3] = (uint8_t)(accum[i] >> 24);
    }

    /* wipe */
    memset(accum, 0, sizeof(accum));
}

/* ---- sponge ---- */

void sponge256_init(sponge256_t *ctx, int domain)
{
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

    for (int i = 0; i < P256_STATE_SIZE; i++)
        ctx->state[i] = fe256_from_u64(IV[i]);

    ctx->state[15] = fe256_add(ctx->state[15], fe256_from_u64((uint64_t)domain));

    memset(ctx->buf, 0, sizeof(ctx->buf));
    ctx->buf_len   = 0;
    ctx->total_len = 0;
}

static void bytes_to_block256(fe256_t block[P256_RATE], const uint8_t *bytes)
{
    for (int i = 0; i < P256_RATE; i++)
        block[i] = fe256_from_bytes(bytes + i * 32);
}

void sponge256_update(sponge256_t *ctx, const uint8_t *data, size_t len)
{
    size_t block_bytes = P256_RATE * 32;  /* 256 bytes */
    ctx->total_len += len;

    if (ctx->buf_len > 0) {
        size_t space = block_bytes - ctx->buf_len;
        size_t copy  = len < space ? len : space;
        memcpy(ctx->buf + ctx->buf_len, data, copy);
        ctx->buf_len += copy;
        data += copy;
        len  -= copy;

        if (ctx->buf_len == block_bytes) {
            fe256_t block[P256_RATE];
            bytes_to_block256(block, ctx->buf);
            absorb256(ctx->state, block);
            ctx->buf_len = 0;
        }
    }

    while (len >= block_bytes) {
        fe256_t block[P256_RATE];
        bytes_to_block256(block, data);
        absorb256(ctx->state, block);
        data += block_bytes;
        len  -= block_bytes;
    }

    if (len > 0) {
        memcpy(ctx->buf, data, len);
        ctx->buf_len = len;
    }
}

void sponge256_final(sponge256_t *ctx, uint8_t out[32])
{
    size_t block_bytes = P256_RATE * 32;
    uint64_t bit_len = ctx->total_len * 8;

    /* padding: 0x80 || 0x00... || 8-byte big-endian bit length */
    ctx->buf[ctx->buf_len++] = 0x80;

    if (ctx->buf_len > block_bytes - 8) {
        memset(ctx->buf + ctx->buf_len, 0, block_bytes - ctx->buf_len);
        fe256_t block[P256_RATE];
        bytes_to_block256(block, ctx->buf);
        absorb256(ctx->state, block);
        ctx->buf_len = 0;
    }

    memset(ctx->buf + ctx->buf_len, 0, block_bytes - 8 - ctx->buf_len);
    for (int i = 0; i < 8; i++)
        ctx->buf[block_bytes - 8 + i] = (uint8_t)(bit_len >> (56 - 8 * i));

    fe256_t block[P256_RATE];
    bytes_to_block256(block, ctx->buf);
    absorb256(ctx->state, block);

    /* one extra permutation */
    permute256(ctx->state);

    /* ARX finalization — uses ALL 256 bits per element */
    arx_finalize256(ctx->state, out);

    /* wipe */
    for (int i = 0; i < P256_STATE_SIZE; i++)
        fe256_wipe(&ctx->state[i]);
    memset(ctx->buf, 0, sizeof(ctx->buf));
}
