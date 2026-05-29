#ifndef SPONGE256_H
#define SPONGE256_H

#include "field256.h"
#include "permutation256.h"

#define ODE_HASH256  0
#define ODE_MAC256   1
#define ODE_KDF256   2

typedef struct {
    fe256_t state[P256_STATE_SIZE];
    uint8_t buf[P256_RATE * 32];  /* 256 bytes */
    size_t  buf_len;
    size_t  total_len;
} sponge256_t;

void sponge256_init(sponge256_t *ctx, int domain);
void sponge256_update(sponge256_t *ctx, const uint8_t *data, size_t len);
void sponge256_final(sponge256_t *ctx, uint8_t out[32]);

#endif
