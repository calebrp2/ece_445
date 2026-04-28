#ifndef INC_DAC_H_
#define INC_DAC_H_

#include "stm32g4xx_hal.h"
#include <stdint.h>

extern DAC_HandleTypeDef hdac1;

void DAC_SignalGen_Init(void);
void DAC_SignalGen_Write(uint16_t value);  /* 12-bit, 0–4095 → 0–3.3 V */

#endif /* INC_DAC_H_ */
