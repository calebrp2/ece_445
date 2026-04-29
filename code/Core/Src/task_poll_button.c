#include "task_poll_button.h"
#include "usbd_cdc_if.h"
#include <string.h>

/* Buttons are active LOW — pressed when the pin reads 0. */
#define BTN_PRESSED(pin) \
    (HAL_GPIO_ReadPin(BTN_GPIO, (pin)) == GPIO_PIN_RESET)

void Task_PollButton_Run(void *argument)
{
    (void)argument;

    bool prev_enter = true;
    bool prev_menu1 = true;
    bool prev_menu2 = true;
    bool prev_menu3 = true;
    bool prev_up    = true;
    bool prev_down  = true;
    bool prev_left  = true;
    bool prev_right = true;
    int counter = 0;

#define Y_PAN_STEP  200                        /* ADC counts per press (~5% of full range) */
#define X_PAN_STEP  (ADC_BUFFER_SIZE / 8)      /* samples per press (12.5% of buffer)     */

    for (;;)
    {
        g_btn.menu1 = !BTN_PRESSED(BTN_MENU1_PIN);
        g_btn.menu2 = !BTN_PRESSED(BTN_MENU2_PIN);
        g_btn.menu3 = !BTN_PRESSED(BTN_MENU3_PIN);
        g_btn.enter = !BTN_PRESSED(BTN_ENTER_PIN);
        g_btn.up    = !BTN_PRESSED(BTN_UP_PIN);
        g_btn.down  = !BTN_PRESSED(BTN_DOWN_PIN);
        g_btn.left  = !BTN_PRESSED(BTN_LEFT_PIN);
        g_btn.right = !BTN_PRESSED(BTN_RIGHT_PIN);

        if (!prev_enter && g_btn.enter)
        {
            g_paused = !g_paused;
            if (g_paused) {
                HAL_TIM_Base_Stop(&htim_adc);
                HAL_ADC_Stop_DMA(&hadc2);
            } else {
                HAL_ADC_Start_DMA(&hadc2, (uint32_t *)g_voltage_buf, ADC_BUFFER_SIZE);
                HAL_TIM_Base_Start(&htim_adc);
            }
            const char *msg = g_paused ? "STATE: PAUSED\r\n" : "STATE: PLAYING\r\n";
            CDC_Transmit_FS((uint8_t *)msg, (uint16_t)strlen(msg));
        }
        prev_enter = g_btn.enter;

        if (!prev_menu1 && g_btn.menu1)
        {
            if (g_zoom_level < 3) g_zoom_level++;
            char buf[24];
            int len = snprintf(buf, sizeof(buf), "ZOOM: %dx\r\n",
                               1 << (g_zoom_level > 0 ? g_zoom_level : 0));
            CDC_Transmit_FS((uint8_t *)buf, (uint16_t)len);
            DisplayCmd_t cmd = { .type = DISP_CMD_WAVEFORM, .param = 0 };
            osMessageQueuePut(xDisplayCmdQueue, &cmd, 0, 0);
        }

        prev_menu1 = g_btn.menu1;

        if (!prev_menu3 && g_btn.menu3)
        {
            /* Cycle: 1 (Current) → 2 (Voltage) → 3 (DAC) → 1 */
            if      (g_sensing_mode == 1) g_sensing_mode = 2;
            else if (g_sensing_mode == 2) g_sensing_mode = 3;
            else                          g_sensing_mode = 1;

            if (g_sensing_mode == 2) {
                /* Voltage: start ADC DMA */
                HAL_ADC_Start_DMA(&hadc2, (uint32_t *)g_voltage_buf, ADC_BUFFER_SIZE);
                HAL_TIM_Base_Start(&htim_adc);
            } else {
                /* Current or DAC: stop ADC DMA */
                HAL_TIM_Base_Stop(&htim_adc);
                HAL_ADC_Stop_DMA(&hadc2);
            }

            const char *mode_name = (g_sensing_mode == 1) ? "CURRENT"
                                  : (g_sensing_mode == 2) ? "VOLTAGE" : "DAC";
            char mode_msg[32];
            int len = snprintf(mode_msg, sizeof(mode_msg), "MODE: %s\r\n", mode_name);
            CDC_Transmit_FS((uint8_t *)mode_msg, (uint16_t)len);

            /* Flush stale display commands so the mode-switch render is next */
            osMessageQueueReset(xDisplayCmdQueue);

            DisplayCmdType_t cmd_type = (g_sensing_mode == 1) ? DISP_CMD_CURRENT
                                      : (g_sensing_mode == 2) ? DISP_CMD_WAVEFORM
                                                              : DISP_CMD_DAC;
            DisplayCmd_t cmd = { .type = cmd_type, .param = 0 };
            osMessageQueuePut(xDisplayCmdQueue, &cmd, 0, 0);
        }
        prev_menu3 = g_btn.menu3;

        if (!prev_menu2 && g_btn.menu2)
        {
            if (g_zoom_level > -3) g_zoom_level--;
            char buf[24];
            int len = snprintf(buf, sizeof(buf), "ZOOM: 1/%dx\r\n",
                               1 << (g_zoom_level < 0 ? -g_zoom_level : 0));
            CDC_Transmit_FS((uint8_t *)buf, (uint16_t)len);
            DisplayCmd_t cmd = { .type = DISP_CMD_WAVEFORM, .param = 0 };
            osMessageQueuePut(xDisplayCmdQueue, &cmd, 0, 0);
        }
        prev_menu2 = g_btn.menu2;

        if (g_sensing_mode == 3) {
            /* DAC mode: up/down = frequency, left/right = amplitude */
            DisplayCmd_t dac_cmd = { .type = DISP_CMD_DAC, .param = 0 };

            if (!prev_left && g_btn.left) {
                if (g_dac_freq_hz < 100U) {
                    g_dac_freq_hz += 5U;
                    if (g_dac_freq_hz > 100U) g_dac_freq_hz = 100U;
                }
                osMessageQueuePut(xDisplayCmdQueue, &dac_cmd, 0, 0);
            }
            if (!prev_down && g_btn.down) {
                if (g_dac_freq_hz > 5U) {
                    g_dac_freq_hz -= 5U;
                    if (g_dac_freq_hz < 5U) g_dac_freq_hz = 5U;
                }
                osMessageQueuePut(xDisplayCmdQueue, &dac_cmd, 0, 0);
            }
            if (!prev_right && g_btn.right) {
                if (g_dac_amp_pct < 100U) {
                    g_dac_amp_pct += 10U;
                    if (g_dac_amp_pct > 100U) g_dac_amp_pct = 100U;
                }
                osMessageQueuePut(xDisplayCmdQueue, &dac_cmd, 0, 0);
            }
            if (!prev_up && g_btn.up) {
                if (g_dac_amp_pct > 10U) {
                    g_dac_amp_pct -= 10U;
                    if (g_dac_amp_pct < 10U) g_dac_amp_pct = 10U;
                }
                osMessageQueuePut(xDisplayCmdQueue, &dac_cmd, 0, 0);
            }
        } else {
            /* Voltage / current mode: up/down = Y pan, left/right = X pan */
            DisplayCmd_t pan_cmd = { .type = DISP_CMD_WAVEFORM, .param = 0 };

            if (!prev_up && g_btn.up) {
                g_pan_y_counts += Y_PAN_STEP;
                osMessageQueuePut(xDisplayCmdQueue, &pan_cmd, 0, 0);
            }
            if (!prev_down && g_btn.down) {
                g_pan_y_counts -= Y_PAN_STEP;
                osMessageQueuePut(xDisplayCmdQueue, &pan_cmd, 0, 0);
            }
            if (!prev_left && g_btn.left) {
                g_pan_x_samples -= X_PAN_STEP;
                if (g_pan_x_samples < 0) g_pan_x_samples = 0;
                osMessageQueuePut(xDisplayCmdQueue, &pan_cmd, 0, 0);
            }
            if (!prev_right && g_btn.right) {
                g_pan_x_samples += X_PAN_STEP;
                if (g_pan_x_samples > (int32_t)(ADC_BUFFER_SIZE - ILI9341_WIDTH))
                    g_pan_x_samples = (int32_t)(ADC_BUFFER_SIZE - ILI9341_WIDTH);
                osMessageQueuePut(xDisplayCmdQueue, &pan_cmd, 0, 0);
            }
        }

        prev_up    = g_btn.up;
        prev_down  = g_btn.down;
        prev_left  = g_btn.left;
        prev_right = g_btn.right;
        counter++;

        // if ((g_btn.menu1 || g_btn.menu2 || g_btn.menu3 ||
        //     g_btn.enter || g_btn.up    || g_btn.down   ||
        //     g_btn.left  || g_btn.right ) && counter == 4)
        // {
        //     counter = 0;
        //     char mode_msg[32];
        //     int len = snprintf(mode_msg, sizeof(mode_msg), "MODE: %s\r\n",
        //                        (g_sensing_mode == 1) ? "CURRENT"
        //                      : (g_sensing_mode == 2) ? "VOLTAGE" : "DAC");
        //     CDC_Transmit_FS((uint8_t *)mode_msg, (uint16_t)len);
        //     char buf[64] = "BTN:";
        //     if (g_btn.menu1) strcat(buf, " MENU1");
        //     if (g_btn.menu2) strcat(buf, " MENU2");
        //     if (g_btn.menu3) strcat(buf, " MENU3");
        //     if (g_btn.enter) strcat(buf, " ENTER");
        //     if (g_btn.up)    strcat(buf, " UP");
        //     if (g_btn.down)  strcat(buf, " DOWN");
        //     if (g_btn.left)  strcat(buf, " LEFT");
        //     if (g_btn.right) strcat(buf, " RIGHT");
        //     strcat(buf, g_paused ? " | PAUSED\r\n" : " | PLAYING\r\n");
        //     CDC_Transmit_FS((uint8_t *)buf, (uint16_t)strlen(buf));
        // }

        osDelay(10);
    }
}
