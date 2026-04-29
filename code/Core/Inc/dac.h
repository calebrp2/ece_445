#ifndef INC_DAC_H_
#define INC_DAC_H_

#include "stm32g4xx_hal.h"
#include <stdint.h>

extern DAC_HandleTypeDef hdac1;

void     DAC_SignalGen_Init(void);
void     DAC_SignalGen_SetFreqAmp(uint32_t freq_hz, uint32_t amp_pct);
void     DAC_SignalGen_Write(uint16_t value);
uint16_t DAC_SignalGen_GetSample(uint32_t idx);

#endif /* INC_DAC_H_ */
