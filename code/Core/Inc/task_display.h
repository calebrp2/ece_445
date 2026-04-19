/**
 * @file    task_display.h
 * @brief   ILI9341 display rendering task — NORMAL priority.
 *
 * Blocks on xDisplayCmdQueue. Renders waveform, FFT, curve-fit, and status
 * onto the 320x240 ILI9341 panel via SPI2.
 */

#ifndef INC_TASK_DISPLAY_H_
#define INC_TASK_DISPLAY_H_

#include "app_globals.h"

#ifdef __cplusplus
extern "C" {
#endif

void Task_Display_Init(void);
void Task_Display_Run(void *argument);

#ifdef __cplusplus
}
#endif

#endif /* INC_TASK_DISPLAY_H_ */
