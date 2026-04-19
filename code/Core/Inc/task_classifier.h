/**
 * @file    task_classifier.h
 * @brief   Wave-type classifier task — sine vs square.
 *
 * Runs a 3-feature logistic regression on the FFT magnitude spectrum
 * produced by Task_FFT.  Result is stored in g_wave_type / g_wave_confidence.
 */

#ifndef INC_TASK_CLASSIFIER_H_
#define INC_TASK_CLASSIFIER_H_

#include "app_globals.h"

#ifdef __cplusplus
extern "C" {
#endif

void Task_Classifier_Init(void);
void Task_Classifier_Run(void *argument);

#ifdef __cplusplus
}
#endif

#endif /* INC_TASK_CLASSIFIER_H_ */
