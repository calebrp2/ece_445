/**
 * @file    task_fft.c
 * @brief   512-point iterative Cooley-Tukey FFT task.
 *
 * Algorithm reference: T. H. Cormen et al., "Introduction to Algorithms,"
 * 3rd ed., Ch. 30 — Iterative-FFT pseudocode.
 *
 * The implementation uses a custom Complex32_t struct to avoid relying on
 * <complex.h>, which may not be present in all ARM-GCC toolchain builds.
 * The Cortex-M4 FPU accelerates float multiply-add operations.
 *
 * Timing probe: DEBUG_FFT_PIN (PB3) is set HIGH before and LOW after the
 * FFT core.  Measure the pulse width to verify the < 100 ms requirement.
 */

#include "task_fft.h"
#include "board_config.h"
#include "usbd_cdc_if.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

/* =========================================================================
 * Complex number arithmetic (avoids <complex.h> dependency)
 * ========================================================================= */
typedef struct { float r; float i; } Complex32_t;

static inline Complex32_t cx_add(Complex32_t a, Complex32_t b)
{ return (Complex32_t){ a.r + b.r, a.i + b.i }; }

static inline Complex32_t cx_sub(Complex32_t a, Complex32_t b)
{ return (Complex32_t){ a.r - b.r, a.i - b.i }; }

static inline Complex32_t cx_mul(Complex32_t a, Complex32_t b)
{ return (Complex32_t){ a.r*b.r - a.i*b.i, a.r*b.i + a.i*b.r }; }

/* =========================================================================
 * FFT workspace — static so it does not consume task stack space
 * ========================================================================= */
#define FFT_N  ADC_BUFFER_SIZE   /* Must be a power of 2 */

static Complex32_t s_fft_buf[FFT_N];

/* =========================================================================
 * Bit-reversal permutation (in-place)
 * ========================================================================= */
static void bit_reverse_copy(const Complex32_t *src, Complex32_t *dst,
                              uint32_t n)
{
    uint32_t log2n = 0;
    for (uint32_t tmp = n; tmp > 1U; tmp >>= 1U) log2n++;

    for (uint32_t k = 0; k < n; k++) {
        uint32_t rev = 0;
        uint32_t kk  = k;
        for (uint32_t b = 0; b < log2n; b++) {
            rev = (rev << 1U) | (kk & 1U);
            kk >>= 1U;
        }
        dst[rev] = src[k];
    }
}

/* =========================================================================
 * Iterative Cooley-Tukey FFT (in-place on dst[], after bit-reversal)
 * Following Algorithm ITERATIVE-FFT from Cormen et al. Fig. 30.4
 * ========================================================================= */
static void iterative_fft(Complex32_t *a, uint32_t n)
{
    /* s = 1 to log2(n): each stage doubles the butterfly size m = 2^s */
    for (uint32_t s = 1U; (1U << s) <= n; s++) {
        uint32_t m = 1U << s;
        /* Principal m-th root of unity: omega_m = e^(-j * 2*pi / m) */
        float angle_m = -2.0f * (float)M_PI / (float)m;
        Complex32_t omega_m = { cosf(angle_m), sinf(angle_m) };

        for (uint32_t k = 0; k < n; k += m) {
            Complex32_t omega = { 1.0f, 0.0f };   /* omega = 1 initially */
            for (uint32_t j = 0; j < m / 2U; j++) {
                Complex32_t t = cx_mul(omega, a[k + j + m / 2U]);
                Complex32_t u = a[k + j];
                a[k + j]             = cx_add(u, t);
                a[k + j + m / 2U]    = cx_sub(u, t);
                omega = cx_mul(omega, omega_m);
            }
        }
    }
}

/* =========================================================================
 * Magnitude computation and peak detection
 * ========================================================================= */
static void compute_magnitude(const Complex32_t *cx, float *mag, uint32_t bins)
{
    float peak_mag  = 0.0f;
    uint32_t peak_k = 0U;

    for (uint32_t k = 1; k < bins; k++) {  /* Skip bin 0 (DC) */
        float m = sqrtf(cx[k].r * cx[k].r + cx[k].i * cx[k].i);
        m /= (float)FFT_N;   /* Normalise */
        mag[k] = m;
        if (m > peak_mag) {
            peak_mag = m;
            peak_k   = k;
        }
    }

    /* Convert bin index to frequency.
     * Frequency resolution = sample_rate / N  (Hz per bin) */
    float freq_res = (float)ADC_SAMPLE_RATE_HZ / (float)FFT_N;
    g_fft_peak_freq_hz = (float)peak_k * freq_res;
    g_fft_peak_mag     = peak_mag;
}

/* =========================================================================
 * Task
 * ========================================================================= */
void Task_FFT_Init(void)
{
    /* DEBUG_FFT_PIN (PB3) is already configured as GPIO_Output in MX_GPIO_Init */
    HAL_GPIO_WritePin(DEBUG_FFT_GPIO, DEBUG_FFT_PIN, GPIO_PIN_RESET);
}

void Task_FFT_Run(void *argument)
{
    (void)argument;

    DisplayCmd_t cmd = { .type = DISP_CMD_FFT, .param = 0 };

    for (;;)
    {
        /* Wait for a fresh ADC buffer from the voltage ADC task */
        osThreadFlagsWait(NOTIF_ADC_BUF_READY, osFlagsWaitAny, osWaitForever);

        /* === Timing probe: set PB3 HIGH === */
        HAL_GPIO_WritePin(DEBUG_FFT_GPIO, DEBUG_FFT_PIN, GPIO_PIN_SET);

        uint32_t hz_i = (uint32_t)g_fft_peak_freq_hz;
        uint32_t hz_f = (uint32_t)((g_fft_peak_freq_hz - (float)hz_i) * 10.0f);
        char fft_log[40];
        int flen = snprintf(fft_log, sizeof(fft_log), "FFT: running (%lu.%lu Hz)\r\n",
                            hz_i, hz_f);
        CDC_Transmit_FS((uint8_t *)fft_log, (uint16_t)flen);

        /* Copy voltage samples into complex input, removing DC offset */
        osMutexAcquire(xADCBufMutex, osWaitForever);
        float mean = 0.0f;
        for (uint32_t i = 0; i < FFT_N; i++) {
            mean += (float)g_voltage_buf[i];
        }
        mean /= (float)FFT_N;
        for (uint32_t i = 0; i < FFT_N; i++) {
            s_fft_buf[i].r = ((float)g_voltage_buf[i] - mean) / (float)ADC_MAX_COUNT;
            s_fft_buf[i].i = 0.0f;
        }
        osMutexRelease(xADCBufMutex);

        /* Bit-reversal copy, then in-place FFT */
        static Complex32_t tmp[FFT_N];
        bit_reverse_copy(s_fft_buf, tmp, FFT_N);
        iterative_fft(tmp, FFT_N);

        /* Compute magnitude for the positive-frequency half (bins 0..N/2-1) */
        osMutexAcquire(xFFTBufMutex, osWaitForever);
        compute_magnitude(tmp, g_fft_mag, FFT_N / 2U);
        osMutexRelease(xFFTBufMutex);

        /* === Timing probe: set PB3 LOW === */
        HAL_GPIO_WritePin(DEBUG_FFT_GPIO, DEBUG_FFT_PIN, GPIO_PIN_RESET);

        /* Notify curve-fit task that new FFT data / voltage data is ready */
        if (hCurveFitTask != NULL) {
            osThreadFlagsSet(hCurveFitTask, NOTIF_ADC_BUF_READY);
        }

        /* Request FFT spectrum redraw */
        osMessageQueuePut(xDisplayCmdQueue, &cmd, 0, 0);
    }
}
