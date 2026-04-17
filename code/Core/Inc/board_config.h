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

/* =========================================================================
 * ADC DMA — DMA1 Channel 1 via DMAMUX (request changes with active ADC)
 * ========================================================================= */
#define ADC_DMA_CHANNEL         DMA1_Channel1
#define ADC_DMA_IRQn            DMA1_Channel1_IRQn
#define ADC_DMA_IRQ_PRIORITY    5   /* >= configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY */

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

#endif /* INC_BOARD_CONFIG_H_ */
