# OdeHash v5 — C Implementation

A cryptographic hash function based on **coupled Riccati ordinary differential equations** over the prime field F_(2^255 - 19).

## Quick Start

```bash
make
```

```bash
./build/ode_hash_v5 "Hello, World!"
```

```bash
./build/ode_hash_v5 -f path/to/file
```

```bash
./build/ode_hash_v5 -f path/to/file -p
```

```bash
./build/ode_hash_v5 -f path/to/file --format
```

## Test

```bash
make test
```

```bash
make bench
```

```bash
make stream
```

```bash
make consttime
```

## How It Works

Input → Padding → Sponge(absorb 256-byte blocks)

- Permutation: 8 rounds × 16 Taylor steps
  - Coupled Riccati recurrence mod p
  - Feistel feedback
  - Round constants
- ARX Finalization: 4 passes × 12 rounds
  - Uses ALL 256 bits per element

### Round Constants

The round constants are the first 8 words of the SHA-256 initial hash values:

<pre>
0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a
0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
</pre>

These are "nothing-up-my-sleeve" numbers — chosen from a well-known public
source so nobody can accuse the designer of picking constants with a hidden
backdoor. **OdeHash does NOT use SHA-256 in any way.** The constants are
only used to break symmetry in the ARX mixing rounds.

## Comparison with Standard Hash Functions

| Property | SHA-256 | SHA-3 (Keccak) | BLAKE3 | **OdeHash v5** |
|---|---|---|---|---|
| **Core primitive** | Merkle-Damgård + Davies-Meyer | Sponge + Keccak-f[1600] | ARX + Merkle tree | Sponge + Coupled Riccati ODE |
| **Non-linear layer** | S-boxes (AES-like) | Keccak χ step | ARX quarter-round | Taylor recurrence of non-linear ODE |
| **Field** | GF(2) (bitwise) | GF(2) (bitwise) | Integers mod 2³² | F_(2²⁵⁵ − 19) (prime field) |
| **Rounds** | 64 | 24 | 7 | 8 × 16 = 128 Taylor steps |
| **State size** | 256 bits | 1600 bits | 256 bits | 4096 bits (16 × 256-bit elements) |
| **Output size** | 256 bits | 256/512 bits | 256 bits | 256 bits |
| **Collision resistance** | 128-bit | 128/256-bit | 128-bit | 128-bit |
| **Constant-time** | Partial (hardware) | Yes | Yes | Yes (branchless field ops) |
| **Domain separation** | No (HMAC needed) | Yes (capacity) | Yes | Yes (ODE_HASH/MAC/KDF) |
| **Performance** | ~500 MB/s | ~300 MB/s | ~1 GB/s | ~30 KB/s (C), ~100 KB/s (Rust) |
| **Security basis** | AES-like diffusion | Keccak permutation | ChaCha core | Inverting coupled Riccati system over F_p |

### Why OdeHash is Different

| Aspect | Standard hashes | OdeHash |
|---|---|---|
| **Non-linearity source** | S-boxes or modular arithmetic | Quadratic ODE recurrence (y' = P + Qy + Ry² + Cy₁y₂) |
| **Confusion** | Fixed S-box lookups | 128 coupled non-linear operations per permutation |
| **Diffusion** | Bitwise mixing | Feistel feedback across 16 state elements |
| **Inversion difficulty** | Known algebraic structure | Requires solving inverse ODE problem over F_p |
| **Singularities** | N/A | Radius of convergence depends on all 64 polynomial coefficients |

### What "Nothing-Up-My-Sleeve" Means

The round constants come from SHA-256's initial values. This is standard
practice in cryptography — BLAKE3 uses them, ChaCha uses √2/√3 digits,
Keccak uses LFSR sequences. The point is: **the designer didn't choose
these constants**, so they can't have hidden a backdoor.

OdeHash's security comes from the ODE permutation, not from the constants.

## Test Vectors

<pre>
""        → 9cac1f68a3a83eeee5422d8d87fcf3e309234196c19505f858856b24ab584544
"a"       → bb15f94e90d514d958ff89d4c2dd233f2319f19f201cdf8933a1ea89194de5f0
"abc"     → fc78a24fbbc41b17ad3a51f53da24cb18511c2c402cbca2038c22a4ba1a49d34
"hello"   → 1d145847cf586aeb8a868be4074c42d59190904c1f8c8b6dc24e332dc6aad674
"test123" → bb3fd667b857780008b7bad5275010f498885334a8e4bd59ade281b86fe30844
</pre>

## Statistical Test Results

Run on 65,536 bytes of hash output (sequential integer inputs):

```bash
python stat_tests.py dieharder_stream.bin
```

| Test | Result | Threshold |
|---|---|---|
| Byte distribution (chi-squared) | 255.93 | < 310 |
| Monobit (z-score) | -1.0330 | < 3.5 |
| Runs test (z-score) | 0.4075 | < 3.5 |
| Serial correlation | -0.002048 | < 0.01 |
| Shannon entropy | 7.9972 bits/byte | > 7.99 |
| Birthday spacings | z=-3.1748 | < 3.5 |
| Poker test | 13107/13107 unique | exact |
| Bit balance | max |z|=0.8906 | < 3.5 |

**Result: 8/8 tests passed**

## Dependencies

- GCC (or Clang)
- GMP library (`sudo apt install libgmp-dev`)

## License

This project is licensed under the [MIT License](LICENSE).
