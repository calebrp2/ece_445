/**
 * @file    waveform_classifier.c
 * @brief   Feature extraction for the RF waveform classifier.
 *
 * Computes 7 features from the current ADC buffer and FFT magnitude array:
 *   - Crest factor, THD, symmetry (time-domain, from adc_buf)
 *   - H2/H1, H3/H1, H4/H1 ratios, spectral rolloff bin (frequency-domain, from g_fft_mag)
 *
 * Called by RF_Classify() in rf_waveform_classifier.c after each FFT update.
 */

#include "waveform_classifier.h"
#include "app_globals.h"
#include <math.h>

void WC_Classify(const uint16_t *adc_buf, WC_Result_t *result)
{
    const uint32_t N    = ADC_BUFFER_SIZE;
    const uint32_t bins = N / 2U;

    /* -----------------------------------------------------------------------
     * Time-domain pass: crest factor and symmetry.
     * Acquire ADC mutex for a consistent snapshot of the buffer.
     * ----------------------------------------------------------------------- */
    osMutexAcquire(xADCBufMutex, osWaitForever);

    float mean = 0.0f;
    for (uint32_t i = 0; i < N; i++)
        mean += (float)adc_buf[i];
    mean /= (float)N;

    float sum_sq = 0.0f;
    float peak   = 0.0f;
    uint32_t above_mean = 0U;
    for (uint32_t i = 0; i < N; i++) {
        float s  = (float)adc_buf[i] - mean;
        float as = s < 0.0f ? -s : s;
        sum_sq += s * s;
        if (as > peak) peak = as;
        if (s > 0.0f)  above_mean++;
    }

    osMutexRelease(xADCBufMutex);

    float rms      = sqrtf(sum_sq / (float)N);
    float crest    = (rms > 1e-6f) ? (peak / rms) : 1.0f;
    float symmetry = (float)above_mean / (float)N;

    /* -----------------------------------------------------------------------
     * Frequency-domain pass: harmonic ratios, THD, spectral rolloff.
     * Reads from g_fft_mag which is written by task_fft and protected by
     * xFFTBufMutex.
     * ----------------------------------------------------------------------- */
    osMutexAcquire(xFFTBufMutex, osWaitForever);

    /* Find the fundamental bin (peak magnitude, skip DC bin 0) */
    float    h1_mag = 0.0f;
    uint32_t h1_bin = 1U;
    for (uint32_t k = 1U; k < bins; k++) {
        if (g_fft_mag[k] > h1_mag) {
            h1_mag = g_fft_mag[k];
            h1_bin = k;
        }
    }

    /* Harmonic magnitudes */
    float h2_mag = (h1_bin * 2U < bins) ? g_fft_mag[h1_bin * 2U] : 0.0f;
    float h3_mag = (h1_bin * 3U < bins) ? g_fft_mag[h1_bin * 3U] : 0.0f;
    float h4_mag = (h1_bin * 4U < bins) ? g_fft_mag[h1_bin * 4U] : 0.0f;

    /* THD: sqrt(sum of H2..Hn squares) / H1 * 100 */
    float harm_sq = 0.0f;
    for (uint32_t n = 2U; n * h1_bin < bins; n++) {
        float hm = g_fft_mag[n * h1_bin];
        harm_sq += hm * hm;
    }
    float thd_pct = (h1_mag > 1e-9f) ? sqrtf(harm_sq) / h1_mag * 100.0f : 0.0f;

    /* Spectral rolloff: smallest bin where cumulative energy >= 85% of total */
    float total_energy = 0.0f;
    for (uint32_t k = 1U; k < bins; k++)
        total_energy += g_fft_mag[k] * g_fft_mag[k];
    float    cumulative  = 0.0f;
    float    threshold   = 0.85f * total_energy;
    uint32_t rolloff_bin = bins - 1U;
    for (uint32_t k = 1U; k < bins; k++) {
        cumulative += g_fft_mag[k] * g_fft_mag[k];
        if (cumulative >= threshold) { rolloff_bin = k; break; }
    }

    osMutexRelease(xFFTBufMutex);

    /* -----------------------------------------------------------------------
     * Pack features
     * ----------------------------------------------------------------------- */
    float inv_h1 = (h1_mag > 1e-9f) ? (1.0f / h1_mag) : 0.0f;

    result->features.crest       = crest;
    result->features.thd_pct     = thd_pct;
    result->features.symmetry    = symmetry;
    result->features.h2_h1_ratio = h2_mag * inv_h1;
    result->features.h3_h1_ratio = h3_mag * inv_h1;
    result->features.h4_h1_ratio = h4_mag * inv_h1;
    result->features.rolloff_bin = rolloff_bin;
}
