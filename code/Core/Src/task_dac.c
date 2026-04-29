/**
 * @file  task_dac.c
 * @brief DAC sine output — software-stepped LUT, one sample per osDelay(1).
 *
 * Frequency is controlled by a Q8 phase accumulator: each millisecond the
 * index advances by freq_hz * 256 * 256 / 1000 Q8-units, so the 256-entry
 * LUT wraps at the correct rate.
 *
 *   5 Hz  → step =  327  (actual 4.99 Hz)
 *  10 Hz  → step =  655  (actual 9.99 Hz)
 * 100 Hz  → step = 6554  (actual 99.98 Hz)
 */

#include "task_dac.h"
#include "dac.h"
#include "app_globals.h"

void Task_DAC_Run(void *argument)
{
    (void)argument;

    uint32_t last_amp = g_dac_amp_pct;
    uint16_t phase_q8 = 0;   /* Q8 fractional LUT index; bits[15:8] = index 0–255 */

    for (;;) {
        uint32_t freq = g_dac_freq_hz;
        uint32_t amp  = g_dac_amp_pct;

        if (amp != last_amp) {
            DAC_SignalGen_SetFreqAmp(freq, amp);
            last_amp = amp;
        }

        DAC_SignalGen_Write(DAC_SignalGen_GetSample(phase_q8 >> 8));

        /* Advance: freq * 256 LUT steps/s at 1000 calls/s, in Q8 units */
        phase_q8 += (uint16_t)(freq * 256U * 256U / 1000U);

        osDelay(1);
    }
}
