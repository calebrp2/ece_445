#include "task_current_adc.h"
#include "usbd_cdc_if.h"
#include <stdio.h>

/* MX_ADC1_Init() (CubeMX) already initialises hadc1 on ADC1.
 * Task_CurrentADC_Init() just switches the channel to IN4 (PA3) and
 * runs calibration — no separate handle needed. */

void Task_CurrentADC_Init(void)
{
    ADC_ChannelConfTypeDef ch = {0};
    ch.Channel      = CURR_ADC_CHANNEL;   /* ADC1_IN4, PA3 */
    ch.Rank         = ADC_REGULAR_RANK_1;
    ch.SamplingTime = ADC_SAMPLETIME_47CYCLES_5;
    ch.SingleDiff   = ADC_SINGLE_ENDED;
    ch.OffsetNumber = ADC_OFFSET_NONE;
    ch.Offset       = 0;
    HAL_ADC_ConfigChannel(&hadc1, &ch);

    HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED);
}

void Task_CurrentADC_Run(void *argument)
{
    (void)argument;

    for (;;)
    {
        if (g_paused || g_sensing_mode != 1)
        {
            osDelay(100);
            continue;
        }

        /* Enable current sense front-end before reading */
        HAL_GPIO_WritePin(CURR_SENSE_EN_GPIO, CURR_SENSE_EN_PIN, GPIO_PIN_SET);
        osDelay(1);   /* brief settle for analog front-end */

        HAL_ADC_Start(&hadc1);
        if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK)
            g_current_raw = (uint16_t)HAL_ADC_GetValue(&hadc1);
        HAL_ADC_Stop(&hadc1);

        /* Disable current sense front-end after reading */
        // HAL_GPIO_WritePin(CURR_SENSE_EN_GPIO, CURR_SENSE_EN_PIN, GPIO_PIN_RESET);

        float ma      = count_to_current_ma(g_current_raw);
        int   ma_int  = (int)ma;
        int   ma_frac = (int)((ma - (float)ma_int) * 100.0f);
        char  log[48];
        int   log_len = snprintf(log, sizeof(log), "CURR: raw=%u  %d.%02dmA\r\n",
                                 (unsigned)g_current_raw, ma_int, ma_frac);
        CDC_Transmit_FS((uint8_t *)log, (uint16_t)log_len);

        DisplayCmd_t cmd = { .type = DISP_CMD_CURRENT, .param = 0 };
        osMessageQueuePut(xDisplayCmdQueue, &cmd, 0, 0);

        /* ~10 Hz sample rate — broken into 10ms chunks so pause is responsive */
        for (int i = 0; i < 10 && !g_paused; i++)
            osDelay(10);
    }
}
