/**
 * @file    task_waveform_class.c
 * @brief   Waveform type classifier task.
 *
 * Classification uses the harmonic content already computed by task_fft.c:
 *
 *   SINE     — low total harmonic distortion (THD < 15 %)
 *   SQUARE   — high THD, but even harmonics are weak (H2+H4 < 12 % of H1)
 *              because a square wave only has odd harmonics
 *   SAWTOOTH — high THD and even harmonics are present (H2+H4 >= 12 % of H1)
 *              because a sawtooth has both odd and even harmonics
 *
 * The task wakes on NOTIF_FFT_DONE set by Task_FFT_Run, reads g_fft_mag[]
 * under xFFTBufMutex, classifies, writes g_waveform_type, and requests a
 * status bar refresh.
 */

#include "task_waveform_class.h"
#include "app_globals.h"
#include "board_config.h"
#include "usbd_cdc_if.h"
#include <math.h>
#include <stdio.h>

/* Minimum normalised peak magnitude to consider signal present */
#define MIN_SIGNAL_MAG    0.005f

/* THD threshold below which the signal is classified as sinusoidal */
#define THD_SINE_THRESH   0.15f

/* Even-harmonic ratio (H2+H4)/H1 below which a high-THD signal is square */
#define EVEN_SQR_THRESH   0.12f

/* Search ±1 bin around a harmonic centre to tolerate slight frequency drift */
static float harmonic_mag(uint32_t bin)
{
    const uint32_t bins = ADC_BUFFER_SIZE / 2U;
    if (bin == 0U || bin >= bins) return 0.0f;
    float m = g_fft_mag[bin];
    if (bin > 1U             && g_fft_mag[bin - 1U] > m) m = g_fft_mag[bin - 1U];
    if (bin + 1U < bins      && g_fft_mag[bin + 1U] > m) m = g_fft_mag[bin + 1U];
    return m;
}

void Task_WaveformClass_Run(void *argument)
{
    (void)argument;

    DisplayCmd_t cmd = { .type = DISP_CMD_STATUS, .param = 0 };

    for (;;) {
        osThreadFlagsWait(NOTIF_FFT_DONE, osFlagsWaitAny, osWaitForever);

        /* Snapshot only the values we need, then release the mutex quickly */
        osMutexAcquire(xFFTBufMutex, osWaitForever);
        float peak_mag  = g_fft_peak_mag;
        float peak_freq = g_fft_peak_freq_hz;
        float freq_res  = (float)ADC_SAMPLE_RATE_HZ / (float)ADC_BUFFER_SIZE;
        uint32_t k1 = (peak_freq > 0.0f)
                      ? (uint32_t)(peak_freq / freq_res + 0.5f)
                      : 0U;
        float h1 = harmonic_mag(k1);
        float h2 = harmonic_mag(2U * k1);
        float h3 = harmonic_mag(3U * k1);
        float h4 = harmonic_mag(4U * k1);
        float h5 = harmonic_mag(5U * k1);
        osMutexRelease(xFFTBufMutex);

        {
            char buf[64];
            int pm_i = (int)peak_mag;
            int pm_f = (int)((peak_mag - (float)pm_i) * 10000.0f);
            int h1_i = (int)h1;
            int h1_f = (int)((h1 - (float)h1_i) * 10000.0f);
            int len  = snprintf(buf, sizeof(buf),
                                "WCLASS: peak_mag=%d.%04d h1=%d.%04d\r\n",
                                pm_i, pm_f, h1_i, h1_f);
            CDC_Transmit_FS((uint8_t *)buf, (uint16_t)len);
        }

        WaveformType_t type;

        if (peak_mag < MIN_SIGNAL_MAG || h1 < MIN_SIGNAL_MAG) {
            type = WAVEFORM_UNKNOWN;
        } else {
            float thd        = sqrtf(h2*h2 + h3*h3 + h4*h4 + h5*h5) / h1;
            float even_ratio = (h2 + h4) / h1;

            if (thd < THD_SINE_THRESH) {
                type = WAVEFORM_SINE;
            } else if (even_ratio < EVEN_SQR_THRESH) {
                type = WAVEFORM_SQUARE;
            } else {
                type = WAVEFORM_SAWTOOTH;
            }
        }

        g_waveform_type = type;

        osMessageQueuePut(xDisplayCmdQueue, &cmd, 0, 0);
    }
}
