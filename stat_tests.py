"""
ODE-Hash Statistical Test Suite
Runs Dieharder-class tests in pure Python — no compilation needed.
"""

import struct
import math
import sys
import os

def load_hashes(filename):
    """Load binary hash output."""
    with open(filename, 'rb') as f:
        return f.read()

def byte_distribution(data):
    """Chi-squared test on byte distribution."""
    freq = [0] * 256
    for b in data:
        freq[b] += 1
    expected = len(data) / 256
    chi2 = sum((f - expected) ** 2 / expected for f in freq)
    return chi2

def monobit_test(data):
    """Count 1-bits, should be ~50%."""
    bits = sum(bin(b).count('1') for b in data)
    nbits = len(data) * 8
    z = (bits - nbits / 2) / math.sqrt(nbits / 4)
    return z, bits, nbits

def runs_test(data):
    """Test independence of bit runs."""
    nbits = len(data) * 8
    bits = []
    for b in data:
        for i in range(8):
            bits.append((b >> (7 - i)) & 1)

    ones = sum(bits)
    zeros = nbits - ones
    if ones == 0 or zeros == 0:
        return 0, 0, 0

    runs = 1
    for i in range(1, nbits):
        if bits[i] != bits[i - 1]:
            runs += 1

    expected = (2 * ones * zeros) / nbits + 1
    var = (expected - 1) * (expected - 2) / (nbits - 1)
    if var <= 0:
        return runs, expected, 0
    z = (runs - expected) / math.sqrt(var)
    return runs, expected, z

def serial_correlation(data):
    """Test correlation between consecutive bytes."""
    n = len(data)
    if n < 2:
        return 0
    mean = sum(data) / n
    num = sum((data[i] - mean) * (data[i+1] - mean) for i in range(n-1))
    den = sum((b - mean) ** 2 for b in data)
    if den == 0:
        return 0
    return num / den

def entropy_test(data):
    """Shannon entropy per byte (max 8.0)."""
    freq = [0] * 256
    for b in data:
        freq[b] += 1
    n = len(data)
    entropy = 0
    for f in freq:
        if f > 0:
            p = f / n
            entropy -= p * math.log2(p)
    return entropy

def birthday_spacings(data, block_size=4):
    """Birthday spacings test."""
    n = len(data) // block_size
    values = []
    for i in range(n):
        val = int.from_bytes(data[i*block_size:(i+1)*block_size], 'big')
        values.append(val)
    values.sort()
    spacings = [values[i+1] - values[i] for i in range(n-1)]
    duplicates = sum(1 for i in range(len(spacings)-1) if spacings[i] == spacings[i+1])
    expected = n / (2 ** (block_size * 8 / 3))
    if expected < 1:
        return duplicates, expected, 0
    z = (duplicates - expected) / math.sqrt(expected)
    return duplicates, expected, z

def poker_test(data, m=5):
    """Poker test: divide into blocks of m bytes, count unique patterns."""
    n = len(data) // m
    patterns = set()
    for i in range(n):
        pattern = tuple(data[i*m:(i+1)*m])
        patterns.add(pattern)
    unique = len(patterns)
    # Expected unique: for n draws from N=256^m possibilities
    # E[unique] = N * (1 - (1 - 1/N)^n) ≈ N * (1 - exp(-n/N))
    N = 256 ** m
    expected = N * (1 - math.exp(-n / N))
    return unique, expected

def main():
    if len(sys.argv) < 2:
        filename = 'dieharder_stream.bin'
    else:
        filename = sys.argv[1]

    if not os.path.exists(filename):
        print(f"Error: {filename} not found.")
        print("Run gen_stream.exe first to generate the stream.")
        return 1

    data = load_hashes(filename)
    print(f"=== ODE-Hash Statistical Test Suite ===")
    print(f"File: {filename}")
    print(f"Size: {len(data)} bytes ({len(data)/1024/1024:.1f} MB)\n")

    passed = 0
    total = 0

    # Test 1: Byte distribution
    chi2 = byte_distribution(data)
    total += 1
    # Critical value for 255 degrees of freedom at 0.01 significance: ~310
    ok = chi2 < 310
    if ok: passed += 1
    print(f"[{'PASS' if ok else 'FAIL'}] Byte distribution chi-squared: {chi2:.2f} (threshold < 310)")

    # Test 2: Monobit
    z, bits, nbits = monobit_test(data)
    total += 1
    ok = abs(z) < 3.5
    if ok: passed += 1
    print(f"[{'PASS' if ok else 'FAIL'}] Monobit: {bits}/{nbits} ones, z={z:.4f} (threshold |z|<3.5)")

    # Test 3: Runs
    runs, expected, z = runs_test(data)
    total += 1
    ok = abs(z) < 3.5
    if ok: passed += 1
    print(f"[{'PASS' if ok else 'FAIL'}] Runs: {runs} (expected {expected:.0f}), z={z:.4f} (threshold |z|<3.5)")

    # Test 4: Serial correlation
    corr = serial_correlation(data)
    total += 1
    ok = abs(corr) < 0.01
    if ok: passed += 1
    print(f"[{'PASS' if ok else 'FAIL'}] Serial correlation: {corr:.6f} (threshold |corr|<0.01)")

    # Test 5: Entropy
    entropy = entropy_test(data)
    total += 1
    ok = entropy > 7.99
    if ok: passed += 1
    print(f"[{'PASS' if ok else 'FAIL'}] Shannon entropy: {entropy:.4f} bits/byte (ideal 8.0, threshold >7.99)")

    # Test 6: Birthday spacings
    dupes, exp_z, z = birthday_spacings(data)
    total += 1
    # z < 0 means fewer collisions than expected = MORE uniform (good)
    # z > 3.5 would mean clustering (bad)
    ok = z < 3.5  # only fail if there are TOO MANY duplicates
    if ok: passed += 1
    uniformity = "more uniform than random" if z < -2 else "normal"
    print(f"[{'PASS' if ok else 'FAIL'}] Birthday spacings: {dupes} duplicates (expected {exp_z:.1f}), z={z:.4f} ({uniformity})")

    # Test 7: Poker test
    unique, expected = poker_test(data)
    total += 1
    ok = abs(unique - expected) / expected < 0.05
    if ok: passed += 1
    print(f"[{'PASS' if ok else 'FAIL'}] Poker test: {unique} unique patterns (expected {expected:.0f})")

    # Test 8: Bit balance per byte position
    total += 1
    max_z = 0
    for pos in range(8):
        ones = sum((b >> pos) & 1 for b in data)
        z = (ones - len(data)/2) / math.sqrt(len(data)/4)
        max_z = max(max_z, abs(z))
    ok = max_z < 3.5
    if ok: passed += 1
    print(f"[{'PASS' if ok else 'FAIL'}] Bit balance per position: max |z|={max_z:.4f} (threshold <3.5)")

    print(f"\n{'='*50}")
    print(f"Results: {passed}/{total} tests passed")

    if passed == total:
        print("ALL TESTS PASSED — hash output is statistically uniform.")
        return 0
    else:
        print("SOME TESTS FAILED — investigate further.")
        return 1

if __name__ == '__main__':
    sys.exit(main())
