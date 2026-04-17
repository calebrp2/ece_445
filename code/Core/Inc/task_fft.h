/**
 * @file    task_fft.h
 * @brief   FFT Analysis task — LOW priority.
 *
 * 512-point iterative Cooley-Tukey FFT on g_voltage_buf[].
 * Posts DISP_CMD_FFT to the display queue.
 */

#ifndef INC_TASK_FFT_H_
#define INC_TASK_FFT_H_

#include "app_globals.h"

#ifdef __cplusplus
extern "C" {
#endif

void Task_FFT_Init(void);
void Task_FFT_Run(void *argument);

#ifdef __cplusplus
}
#endif

#endif /* INC_TASK_FFT_H_ */
