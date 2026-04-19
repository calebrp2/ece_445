/**
 * @file    app_globals.c
 * @brief   Definitions of all shared application buffers, queue handles,
 *          mutex handles, and task handles.
 */

#include "app_globals.h"

/* =========================================================================
 * PERIPHERAL HANDLES
 * hadc2 / hdma_adc2 are owned by CubeMX (main.c). Only htim_adc is ours.
 * ========================================================================= */
TIM_HandleTypeDef  htim_adc;

/* =========================================================================
 * ADC SAMPLE BUFFERS
 * ========================================================================= */
uint16_t g_voltage_buf[ADC_BUFFER_SIZE];
uint32_t g_adc_buf_len = ADC_BUFFER_SIZE;

/* =========================================================================
 * FFT BUFFERS
 * ========================================================================= */
float g_fft_mag[ADC_BUFFER_SIZE / 2];
float g_fft_peak_freq_hz = 0.0f;
float g_fft_peak_mag     = 0.0f;

/* =========================================================================
 * CURVE FIT RESULT
 * ========================================================================= */
CurveFitResult_t g_fit_result = { 0 };

/* =========================================================================
 * WAVE CLASSIFIER RESULT
 * ========================================================================= */
volatile WaveType_t g_wave_type       = WAVE_TYPE_UNKNOWN;
volatile float      g_wave_confidence = 0.0f;

/* =========================================================================
 * APPLICATION STATE
 * ========================================================================= */
volatile MeasMode_t g_meas_mode = MEAS_MODE_LV;

/* =========================================================================
 * FREERTOS HANDLES
 * ========================================================================= */
osMessageQueueId_t xDisplayCmdQueue = NULL;
osMutexId_t        xADCBufMutex     = NULL;
osMutexId_t        xFFTBufMutex     = NULL;

osThreadId_t hVoltageADCTask  = NULL;
osThreadId_t hCurrentADCTask  = NULL;
osThreadId_t hFFTTask         = NULL;
osThreadId_t hDisplayTask     = NULL;
osThreadId_t hCurveFitTask    = NULL;
osThreadId_t hClassifierTask  = NULL;

/* =========================================================================
 * INITIALISATION
 * ========================================================================= */
void App_Globals_Init(void)
{
    xDisplayCmdQueue = osMessageQueueNew(8, sizeof(DisplayCmd_t), NULL);
    xADCBufMutex     = osMutexNew(NULL);
    xFFTBufMutex     = osMutexNew(NULL);
}
