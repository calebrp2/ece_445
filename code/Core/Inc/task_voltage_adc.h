/**
 * @file    task_voltage_adc.h
 * @brief   Voltage ADC task — HIGH priority.
 *
 * Owns ADC1 + TIM2 (trigger) + DMA1 Channel 1.
 * Samples PA0 (ADC1_IN1) at ADC_SAMPLE_RATE_HZ into g_voltage_buf[].
 * Notifies hFFTTask via thread flag when a full buffer is ready.
 * Sends DISP_CMD_WAVEFORM to the display queue.
 */

#ifndef INC_TASK_VOLTAGE_ADC_H_
#define INC_TASK_VOLTAGE_ADC_H_

#include "app_globals.h"

#ifdef __cplusplus
extern "C" {
#endif

void Task_VoltageADC_Init(void);
void Task_VoltageADC_Run(void *argument);
void Task_VoltageADC_DMA_Cplt(DMA_HandleTypeDef *hdma);
void Task_VoltageADC_DMA_HalfCplt(DMA_HandleTypeDef *hdma);

#ifdef __cplusplus
}
#endif

#endif /* INC_TASK_VOLTAGE_ADC_H_ */
