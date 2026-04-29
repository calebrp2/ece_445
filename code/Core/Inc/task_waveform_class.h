/**
 * @file    task_waveform_class.h
 * @brief   Waveform type classifier task (sine / square / sawtooth).
 *
 * Waits for NOTIF_FFT_DONE, reads g_fft_mag[], and classifies by
 * total harmonic distortion (THD) and even/odd harmonic ratio.
 */

#ifndef INC_TASK_WAVEFORM_CLASS_H_
#define INC_TASK_WAVEFORM_CLASS_H_

void Task_WaveformClass_Run(void *argument);

#endif /* INC_TASK_WAVEFORM_CLASS_H_ */
