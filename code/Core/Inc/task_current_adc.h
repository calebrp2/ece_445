/**
 * @file    task_current_adc.h
 * @brief   Current ADC task stub — will own ADC2 on the G4 PCB.
 *
 * PA1 = ADC2_IN2.  Implementation pending hardware bring-up.
 */

#ifndef INC_TASK_CURRENT_ADC_H_
#define INC_TASK_CURRENT_ADC_H_

#include "app_globals.h"

#ifdef __cplusplus
extern "C" {
#endif

void Task_CurrentADC_Init(void);
void Task_CurrentADC_Run(void *argument);

#ifdef __cplusplus
}
#endif

#endif /* INC_TASK_CURRENT_ADC_H_ */
