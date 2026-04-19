/**
 * @file    board_config.h
 * @brief   Hardware pin and peripheral configuration for the PocketScope PCB.
 *
 * Target: STM32G473CETx, 16 MHz HSI (no PLL)
 *
 * To port: update GPIO port/pin defines, peripheral instances, and clock rates.
 */

#ifndef INC_BOARD_CONFIG_H_
#define INC_BOARD_CONFIG_H_

#include "stm32g4xx_hal.h"

/* =========================================================================
 * SYSTEM CLOCK  (16 MHz HSI, no PLL — matches SystemClock_Config in main.c)
 * ========================================================================= */
#define BOARD_SYSCLK_HZ         16000000UL
#define BOARD_APB1_CLK_HZ       16000000UL
#define BOARD_APB2_CLK_HZ       16000000UL

/* =========================================================================
 * SENSE CONTROL — PA2 output selects the analog front-end range
 *   HIGH → low-voltage path active  (read from PA1 / ADC2_IN2)
 *   LOW  → high-voltage path active (read from PA0 / ADC1_IN1)
 * PA2 is already configured as GPIO_OUTPUT_PP by MX_GPIO_Init.
 * ========================================================================= */
#define SENSE_CTRL_GPIO         GPIOA
#define SENSE_CTRL_PIN          GPIO_PIN_2

/* =========================================================================
 * SPI2 — ILI9341 display bus (PB13=SCK AF5, PB15=MOSI AF5)
 * ========================================================================= */
#define DISP_SPI_INSTANCE       SPI2
#define DISP_SPI_SCK_GPIO       GPIOB
#define DISP_SPI_SCK_PIN        GPIO_PIN_13
#define DISP_SPI_MOSI_GPIO      GPIOB
#define DISP_SPI_MOSI_PIN       GPIO_PIN_15
#define DISP_SPI_AF             GPIO_AF5_SPI2

/* ILI9341 control signals */
#define DISP_CS_GPIO            GPIOB
#define DISP_CS_PIN             GPIO_PIN_12
#define DISP_DC_GPIO            GPIOC
#define DISP_DC_PIN             GPIO_PIN_13
#define DISP_RST_GPIO           GPIOA
#define DISP_RST_PIN            GPIO_PIN_7
#define DISP_BL_GPIO            GPIOB
#define DISP_BL_PIN             GPIO_PIN_9

/* ILI9341 geometry */
#define ILI9341_WIDTH           320U
#define ILI9341_HEIGHT          240U

/* =========================================================================
 * CURRENT SENSE ENABLE — PA6 output, HIGH = current path active
 *   Must be driven LOW whenever voltage ADC is running.
 * ========================================================================= */
#define CURR_SENSE_EN_GPIO      GPIOA
#define CURR_SENSE_EN_PIN       GPIO_PIN_6

/* =========================================================================
 * LOW-VOLTAGE ADC — PA1 = ADC2_IN2  (SENSE_CTRL HIGH)
 * ========================================================================= */
#define LV_ADC_INSTANCE         ADC2
#define LV_ADC_CHANNEL          ADC_CHANNEL_2
#define LV_ADC_GPIO             GPIOA
#define LV_ADC_PIN              GPIO_PIN_1

/* =========================================================================
 * HIGH-VOLTAGE ADC — PA0 = ADC1_IN1  (SENSE_CTRL LOW)
 * ========================================================================= */
#define HV_ADC_INSTANCE         ADC1
#define HV_ADC_CHANNEL          ADC_CHANNEL_1
#define HV_ADC_GPIO             GPIOA
#define HV_ADC_PIN              GPIO_PIN_0

/* ADC trigger timer — TIM2 fires at ADC_SAMPLE_RATE_HZ */
#define ADC_TIM_INSTANCE        TIM2

/* Reference voltage in millivolts (3.3 V) */
#define ADC_VREF_MV             3300U
#define ADC_RESOLUTION_BITS     12U
#define ADC_MAX_COUNT           ((1U << ADC_RESOLUTION_BITS) - 1U)  /* 4095 */

/* Sample rate per channel in Hz (target: up to 100 kHz per spec) */
#define ADC_SAMPLE_RATE_HZ      10000U

/* Samples per acquisition frame — must be power of 2 for FFT */
#define ADC_BUFFER_SIZE         512U

/* Debug timing probe — PB3 toggles around FFT core */
#define DEBUG_FFT_GPIO          GPIOB
#define DEBUG_FFT_PIN           GPIO_PIN_3

#endif /* INC_BOARD_CONFIG_H_ */
