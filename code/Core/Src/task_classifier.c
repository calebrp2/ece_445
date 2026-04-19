/**
 * @file    task_classifier.c
 * @brief   Sine / square wave classifier using logistic regression on FFT features.
 *
 * Algorithm
 * ---------
 * Three features are extracted from g_fft_mag[] after every FFT frame:
 *
 *   f_odd  — odd-harmonic ratio: (mag[3f] + mag[5f] + mag[7f]) / mag[f]
 *             Square waves contain odd harmonics at 1/n amplitude
 *             (1/3, 1/5, 1/7 …); sine waves do not.
 *             Expected values: sine ≈ 0.02, square ≈ 0.68
 *
 *   f_even — even-harmonic ratio: (mag[2f] + mag[4f] + mag[6f]) / mag[f]
 *             Neither pure waveform contains even harmonics; high f_even
 *             signals a distorted or asymmetric wave and penalises the score.
 *             Expected values: sine ≈ 0.01, square ≈ 0.01
 *
 *   f_thd  — total harmonic distortion proxy:
 *             sqrt(Σ mag[k]² for k ≠ peak_bin) / mag[peak_bin]
 *             Square wave THD ≈ 0.48; sine wave THD ≈ 0.02.
 *
 * The three features are passed through a single logistic-regression unit:
 *
 *   score = σ(b + w_odd·f_odd + w_even·f_even + w_thd·f_thd)
 *
 *   where σ(x) = 1 / (1 + exp(-x))
 *
 * Weights were derived analytically from the theoretical feature values of
 * ideal sine and square waves and verified to give score > 0.97 for a square
 * and < 0.03 for a sine across 6 dB of amplitude variation:
 *
 *   b      = -4.0
 *   w_odd  = +10.0
 *   w_even =  -3.0
 *   w_thd  =  +6.0
 *
 * Decision threshold: score ≥ 0.5 → SQUARE, else → SINE.
 * The raw score is exported as g_wave_confidence (0–1).
 *
 * Resource budget (STM32G473, 16 MHz)
 * ------------------------------------
 * Feature extraction: O(N/2) float additions and one sqrtf — < 1 ms.
 * Sigmoid: one expf call — < 0.1 ms.
 * Stack: ~128 bytes (all heavy buffers accessed via globals).
 */

#include "task_classifier.h"
#include "app_globals.h"
#include "usbd_cdc_if.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

/* =========================================================================
 * Model weights  (logistic regression, trained on theoretical waveforms)
 * ========================================================================= */
#define CLF_BIAS      (-4.0f)
#define CLF_W_ODD     ( 10.0f)
#define CLF_W_EVEN    ( -3.0f)
#define CLF_W_THD     (  6.0f)

/* Minimum fundamental magnitude to attempt classification (avoids noise) */
#define CLF_MIN_PEAK_MAG   (0.01f)

/* Number of FFT bins in the positive-frequency half */
#define CLF_FFT_BINS   (ADC_BUFFER_SIZE / 2U)

/* =========================================================================
 * Internal helpers
 * ========================================================================= */

static inline float sigmoid(float x)
{
    return 1.0f / (1.0f + expf(-x));
}

/* Return the magnitude at bin index 'bin', clamped to valid range. */
static inline float safe_mag(const float *mag, uint32_t bin)
{
    return (bin < CLF_FFT_BINS) ? mag[bin] : 0.0f;
}

/**
 * Extract the three features from the FFT magnitude buffer.
 *
 * @param mag       Normalised FFT magnitude array (CLF_FFT_BINS entries).
 * @param peak_bin  Index of the fundamental frequency bin.
 * @param f_odd     Output: odd-harmonic ratio.
 * @param f_even    Output: even-harmonic ratio.
 * @param f_thd     Output: THD proxy (RMS of all non-peak bins / peak mag).
 */
static void extract_features(const float *mag, uint32_t peak_bin,
                              float *f_odd, float *f_even, float *f_thd)
{
    float fund = safe_mag(mag, peak_bin);

    /* Avoid division by zero — caller already checked fund >= CLF_MIN_PEAK_MAG */
    if (fund < 1e-9f) {
        *f_odd = *f_even = *f_thd = 0.0f;
        return;
    }

    /* Harmonic bin indices */
    uint32_t bin2 = 2U * peak_bin;
    uint32_t bin3 = 3U * peak_bin;
    uint32_t bin4 = 4U * peak_bin;
    uint32_t bin5 = 5U * peak_bin;
    uint32_t bin6 = 6U * peak_bin;
    uint32_t bin7 = 7U * peak_bin;

    *f_odd  = (safe_mag(mag, bin3) + safe_mag(mag, bin5) + safe_mag(mag, bin7)) / fund;
    *f_even = (safe_mag(mag, bin2) + safe_mag(mag, bin4) + safe_mag(mag, bin6)) / fund;

    /* THD: RMS of every bin except the DC bin (0) and the fundamental */
    float sum_sq = 0.0f;
    for (uint32_t k = 1U; k < CLF_FFT_BINS; k++) {
        if (k == peak_bin) continue;
        sum_sq += mag[k] * mag[k];
    }
    *f_thd = sqrtf(sum_sq) / fund;
}

/* =========================================================================
 * Task
 * ========================================================================= */

void Task_Classifier_Init(void)
{
    g_wave_type       = WAVE_TYPE_UNKNOWN;
    g_wave_confidence = 0.0f;
}

void Task_Classifier_Run(void *argument)
{
    (void)argument;

    /* Local copy of the FFT buffer to minimise mutex hold time */
    static float mag_local[CLF_FFT_BINS];

    for (;;)
    {
        /* Block until the FFT task signals a new frame */
        osThreadFlagsWait(NOTIF_ADC_BUF_READY, osFlagsWaitAny, osWaitForever);

        /* --- Snapshot FFT magnitudes and peak info under mutex --- */
        osMutexAcquire(xFFTBufMutex, osWaitForever);
        memcpy(mag_local, (const void *)g_fft_mag, sizeof(mag_local));
        float peak_freq = g_fft_peak_freq_hz;
        float peak_mag  = g_fft_peak_mag;
        osMutexRelease(xFFTBufMutex);

        /* --- Compute fundamental bin index from peak frequency --- */
        float freq_res = (float)ADC_SAMPLE_RATE_HZ / (float)ADC_BUFFER_SIZE;
        uint32_t peak_bin = (freq_res > 0.0f)
                            ? (uint32_t)(peak_freq / freq_res + 0.5f)
                            : 0U;

        /* Skip classification if signal is too weak or peak is at DC */
        if (peak_mag < CLF_MIN_PEAK_MAG || peak_bin < 1U) {
            g_wave_type       = WAVE_TYPE_UNKNOWN;
            g_wave_confidence = 0.0f;
            continue;
        }

        /* --- Feature extraction --- */
        float f_odd, f_even, f_thd;
        extract_features(mag_local, peak_bin, &f_odd, &f_even, &f_thd);

        /* --- Logistic regression --- */
        float logit = CLF_BIAS
                    + CLF_W_ODD  * f_odd
                    + CLF_W_EVEN * f_even
                    + CLF_W_THD  * f_thd;
        float score = sigmoid(logit);

        /* --- Decision & publish --- */
        WaveType_t result = (score >= 0.5f) ? WAVE_TYPE_SQUARE : WAVE_TYPE_SINE;
        g_wave_type       = result;
        g_wave_confidence = score;   /* 1.0 = certain square, 0.0 = certain sine */

        /* --- USB debug log --- */
        const char *label = (result == WAVE_TYPE_SQUARE) ? "SQUARE" : "SINE";

        /* Express confidence as an integer percentage (no %f in newlib-nano) */
        uint32_t conf_pct = (uint32_t)(score * 100.0f + 0.5f);

        /* Encode features as scaled integers for transmission */
        uint32_t odd_i  = (uint32_t)(f_odd  * 1000.0f);
        uint32_t even_i = (uint32_t)(f_even * 1000.0f);
        uint32_t thd_i  = (uint32_t)(f_thd  * 1000.0f);

        char buf[80];
        int len = snprintf(buf, sizeof(buf),
                           "CLF: %s  conf=%lu%%  odd=%lu.%03lu  thd=%lu.%03lu\r\n",
                           label,
                           conf_pct,
                           odd_i / 1000U, odd_i % 1000U,
                           thd_i / 1000U, thd_i % 1000U);
        (void)even_i; /* available but omitted for line-length */
        CDC_Transmit_FS((uint8_t *)buf, (uint16_t)len);
    }
}
