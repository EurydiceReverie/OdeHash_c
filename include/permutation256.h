#ifndef PERMUTATION256_H
#define PERMUTATION256_H

#include "field256.h"

#define P256_STATE_SIZE    16
#define P256_RATE           8
#define P256_ROUNDS         8
#define P256_STEPS          16

void permute256(fe256_t state[P256_STATE_SIZE]);
void absorb256(fe256_t state[P256_STATE_SIZE], const fe256_t block[P256_RATE]);

#endif
