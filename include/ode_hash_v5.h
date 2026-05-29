#ifndef ODE_HASH_V5_H
#define ODE_HASH_V5_H

#include <stdint.h>
#include <stddef.h>

#define ODE_HASH5_SIZE 32

#define ODE_HASH5  0
#define ODE_MAC5   1
#define ODE_KDF5   2

int ode_hash_v5(const uint8_t *input, size_t len, uint8_t out[32]);

#endif
