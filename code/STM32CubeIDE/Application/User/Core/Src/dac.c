/**
 * @file  dac.c
 * @brief DAC1 channel 1 — PA4, 12-bit, 0–3.3 V.
 *
 * MX_DAC1_Init() (CubeMX) configures the channel (output buffer on, no
 * trigger, external pin).  DAC_SignalGen_Init() just starts the channel.
 * Call after MX_DAC1_Init() in main USER CODE 2.
 */

#include "dac.h"

void DAC_SignalGen_Init(void)
{
    HAL_DAC_Start(&hdac1, DAC_CHANNEL_1);
}

void DAC_SignalGen_Write(uint16_t value)
{
    HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_1, DAC_ALIGN_12B_R, value & 0x0FFFu);
}
