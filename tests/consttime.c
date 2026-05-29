/*
 * Constant-time verification test (dudect-style).
 *
 * Uses QueryPerformanceCounter (Windows) for high-resolution timing.
 * Measures timing variance between two classes of inputs.
 */

#include "ode_hash_v5.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <windows.h>

#define N_SAMPLES 1000
#define INPUT_LEN 32

static double now_ns(void)
{
    static LARGE_INTEGER freq = {0};
    LARGE_INTEGER ticks;
    if (freq.QuadPart == 0)
        QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&ticks);
    return (double)ticks.QuadPart / (double)freq.QuadPart * 1e9;
}

int main(void)
{
    printf("=== Constant-Time Verification (dudect-style) ===\n");
    printf("  Samples: %d\n\n", N_SAMPLES);

    double *times_a = (double *)malloc(N_SAMPLES * sizeof(double));
    double *times_b = (double *)malloc(N_SAMPLES * sizeof(double));
    if (!times_a || !times_b) {
        printf("  Error: malloc failed\n");
        return 1;
    }

    /* Class A: first byte = 0x00 */
    for (int i = 0; i < N_SAMPLES; i++) {
        uint8_t input[INPUT_LEN];
        for (int j = 0; j < INPUT_LEN; j++) input[j] = (uint8_t)(rand() & 0xFF);
        input[0] = 0x00;

        uint8_t out[32];
        double t0 = now_ns();
        ode_hash_v5(input, INPUT_LEN, out);
        double t1 = now_ns();
        times_a[i] = t1 - t0;
    }

    /* Class B: first byte = 0xFF */
    for (int i = 0; i < N_SAMPLES; i++) {
        uint8_t input[INPUT_LEN];
        for (int j = 0; j < INPUT_LEN; j++) input[j] = (uint8_t)(rand() & 0xFF);
        input[0] = 0xFF;

        uint8_t out[32];
        double t0 = now_ns();
        ode_hash_v5(input, INPUT_LEN, out);
        double t1 = now_ns();
        times_b[i] = t1 - t0;
    }

    /* compute means */
    double sum_a = 0, sum_b = 0;
    for (int i = 0; i < N_SAMPLES; i++) {
        sum_a += times_a[i];
        sum_b += times_b[i];
    }
    double mean_a = sum_a / N_SAMPLES;
    double mean_b = sum_b / N_SAMPLES;

    /* compute variances */
    double var_a = 0, var_b = 0;
    for (int i = 0; i < N_SAMPLES; i++) {
        var_a += (times_a[i] - mean_a) * (times_a[i] - mean_a);
        var_b += (times_b[i] - mean_b) * (times_b[i] - mean_b);
    }
    var_a /= (N_SAMPLES - 1);
    var_b /= (N_SAMPLES - 1);

    /* Welch's t-test */
    double se = sqrt(var_a / N_SAMPLES + var_b / N_SAMPLES);
    double t_stat = (mean_a - mean_b) / se;

    printf("  Class A (0x00): mean = %.1f ns, std = %.1f ns\n", mean_a, sqrt(var_a));
    printf("  Class B (0xFF): mean = %.1f ns, std = %.1f ns\n", mean_b, sqrt(var_b));
    printf("  t-statistic: %.4f\n", t_stat);
    printf("  |t| < 4.5 -> constant-time (99.9%% confidence)\n\n");

    free(times_a);
    free(times_b);

    if (fabs(t_stat) < 4.5) {
        printf("  PASS: No statistically significant timing difference.\n");
        return 0;
    } else {
        printf("  FAIL: Timing difference detected (|t| = %.2f).\n", fabs(t_stat));
        return 1;
    }
}
