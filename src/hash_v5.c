#include "ode_hash_v5.h"
#include "sponge256.h"
#include <string.h>

int ode_hash_v5(const uint8_t *input, size_t len, uint8_t out[32])
{
    sponge256_t ctx;
    sponge256_init(&ctx, ODE_HASH5);
    sponge256_update(&ctx, input, len);
    sponge256_final(&ctx, out);
    return 0;
}
