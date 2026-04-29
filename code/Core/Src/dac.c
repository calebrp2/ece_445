/**
 * @file  dac.c
 * @brief DAC1 CH1 sine generator — software-stepped 256-point LUT.
 *
 * Task_DAC_Run calls DAC_SignalGen_Write once per osDelay(1) and advances a
 * Q8 phase accumulator to control frequency.  DAC_SignalGen_SetFreqAmp
 * rebuilds the amplitude-scaled LUT whenever amplitude changes.
 */

#include "dac.h"
#include "app_globals.h"
#include <math.h>

#define DAC_LUT_SIZE  256U

static uint16_t s_dac_lut[DAC_LUT_SIZE];

static void rebuild_lut(uint32_t amp_pct)
{
    float scale = (float)amp_pct / 100.0f;
    for (uint32_t i = 0; i < DAC_LUT_SIZE; i++) {
        float   v   = sinf(2.0f * (float)M_PI * (float)i / (float)DAC_LUT_SIZE);
        int32_t dev = (int32_t)(v * 2047.0f * scale);
        s_dac_lut[i] = (uint16_t)(2048 + dev);
    }
}

void DAC_SignalGen_Init(void)
{
    rebuild_lut(g_dac_amp_pct);
    HAL_DAC_Start(&hdac1, DAC_CHANNEL_1);
}

void DAC_SignalGen_SetFreqAmp(uint32_t freq_hz, uint32_t amp_pct)
{
    (void)freq_hz;
    rebuild_lut(amp_pct);
}

void DAC_SignalGen_Write(uint16_t value)
{
    HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_1, DAC_ALIGN_12B_R, value);
}

uint16_t DAC_SignalGen_GetSample(uint32_t idx)
{
    return s_dac_lut[idx & (DAC_LUT_SIZE - 1U)];
}
