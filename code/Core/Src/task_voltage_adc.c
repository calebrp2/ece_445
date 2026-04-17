/**
 * @file    task_voltage_adc.c
 * @brief   Voltage ADC task — reads low-voltage sense (PA1 / ADC2_IN2).
 *
 * SENSE_CTRL (PA2) is driven HIGH to activate the low-voltage path.
 * ADC2 is triggered by TIM2 TRGO at ADC_SAMPLE_RATE_HZ.
 * DMA1 Channel 1 fills g_voltage_buf[] in circular mode.
 *
 * To switch to high-voltage sense: drive SENSE_CTRL LOW and
 * reconfigure for HV_ADC_INSTANCE / HV_ADC_CHANNEL (PA0 / ADC1_IN1).
 */

#include "task_voltage_adc.h"
#include "task_fft.h"

/* -------------------------------------------------------------------------
 * DMA callbacks
 * ------------------------------------------------------------------------- */

void Task_VoltageADC_DMA_HalfCplt(DMA_HandleTypeDef *hdma)
{
    (void)hdma;
}

void Task_VoltageADC_DMA_Cplt(DMA_HandleTypeDef *hdma)
{
    (void)hdma;

    if (hVoltageADCTask != NULL) {
        osThreadFlagsSet(hVoltageADCTask, NOTIF_ADC_BUF_READY);
    }
}

/* -------------------------------------------------------------------------
 * Peripheral initialisation — low-voltage sense path
 * ------------------------------------------------------------------------- */

void Task_VoltageADC_Init(void)
{
    /* Assert SENSE_CTRL HIGH to enable low-voltage front-end (PA1) */
    HAL_GPIO_WritePin(SENSE_CTRL_GPIO, SENSE_CTRL_PIN, GPIO_PIN_SET);

    /* --- ADC2: LV sense, PA1 = ADC2_IN2, triggered by TIM2 TRGO --- */
    __HAL_RCC_ADC12_CLK_ENABLE();
    HAL_ADC_DeInit(&hadc1);

    hadc1.Instance                   = LV_ADC_INSTANCE;
    hadc1.Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc1.Init.Resolution            = ADC_RESOLUTION_12B;
    hadc1.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    hadc1.Init.GainCompensation      = 0;
    hadc1.Init.ScanConvMode          = ADC_SCAN_DISABLE;
    hadc1.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;
    hadc1.Init.LowPowerAutoWait      = DISABLE;
    hadc1.Init.ContinuousConvMode    = DISABLE;
    hadc1.Init.NbrOfConversion       = 1;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv      = ADC_EXTERNALTRIG_T2_TRGO;
    hadc1.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_RISING;
    hadc1.Init.DMAContinuousRequests = ENABLE;
    hadc1.Init.Overrun               = ADC_OVR_DATA_OVERWRITTEN;
    hadc1.Init.OversamplingMode      = DISABLE;
    HAL_ADC_Init(&hadc1);

    ADC_ChannelConfTypeDef ch = {0};
    ch.Channel      = LV_ADC_CHANNEL;   /* ADC2_IN2, PA1 */
    ch.Rank         = ADC_REGULAR_RANK_1;
    ch.SamplingTime = ADC_SAMPLETIME_47CYCLES_5;
    ch.SingleDiff   = ADC_SINGLE_ENDED;
    ch.OffsetNumber = ADC_OFFSET_NONE;
    ch.Offset       = 0;
    HAL_ADC_ConfigChannel(&hadc1, &ch);

    HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED);

    /* --- DMA1 Channel 1, DMAMUX request ADC2 --- */
    __HAL_RCC_DMA1_CLK_ENABLE();
    __HAL_RCC_DMAMUX1_CLK_ENABLE();

    hdma_adc1.Instance                 = ADC_DMA_CHANNEL;
    hdma_adc1.Init.Request             = DMA_REQUEST_ADC2;
    hdma_adc1.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    hdma_adc1.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_adc1.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_adc1.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_adc1.Init.MemDataAlignment    = DMA_MDATAALIGN_HALFWORD;
    hdma_adc1.Init.Mode                = DMA_CIRCULAR;
    hdma_adc1.Init.Priority            = DMA_PRIORITY_HIGH;
    HAL_DMA_Init(&hdma_adc1);

    __HAL_LINKDMA(&hadc1, DMA_Handle, hdma_adc1);

    HAL_NVIC_SetPriority(ADC_DMA_IRQn, ADC_DMA_IRQ_PRIORITY, 0);
    HAL_NVIC_EnableIRQ(ADC_DMA_IRQn);

    /* --- TIM2: TRGO on update event at ADC_SAMPLE_RATE_HZ --- */
    __HAL_RCC_TIM2_CLK_ENABLE();

    htim_adc.Instance               = ADC_TIM_INSTANCE;
    htim_adc.Init.Prescaler         = 0;
    htim_adc.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim_adc.Init.Period            = (BOARD_APB1_CLK_HZ / ADC_SAMPLE_RATE_HZ) - 1U;
    htim_adc.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim_adc.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_Base_Init(&htim_adc);

    TIM_MasterConfigTypeDef mc = {0};
    mc.MasterOutputTrigger = TIM_TRGO_UPDATE;
    mc.MasterSlaveMode     = TIM_MASTERSLAVEMODE_DISABLE;
    HAL_TIMEx_MasterConfigSynchronization(&htim_adc, &mc);

    HAL_ADC_Start_DMA(&hadc1, (uint32_t *)g_voltage_buf, ADC_BUFFER_SIZE);
    HAL_TIM_Base_Start(&htim_adc);
}

/* -------------------------------------------------------------------------
 * Task function
 * ------------------------------------------------------------------------- */

void Task_VoltageADC_Run(void *argument)
{
    (void)argument;

    DisplayCmd_t cmd = { .type = DISP_CMD_WAVEFORM, .param = 0 };

    for (;;)
    {
        osThreadFlagsWait(NOTIF_ADC_BUF_READY, osFlagsWaitAny, osWaitForever);

        if (hFFTTask != NULL) {
            osThreadFlagsSet(hFFTTask, NOTIF_ADC_BUF_READY);
        }

        osMessageQueuePut(xDisplayCmdQueue, &cmd, 0, 0);
    }
}

/* HAL weak callback overrides — dispatched from DMA1_Channel1_IRQHandler */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    (void)hadc;
    Task_VoltageADC_DMA_Cplt(NULL);
}

void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc)
{
    (void)hadc;
    Task_VoltageADC_DMA_HalfCplt(NULL);
}
