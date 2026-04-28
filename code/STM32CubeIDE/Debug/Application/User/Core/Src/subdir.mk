################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Application/User/Core/Src/app_freertos.c \
../Application/User/Core/Src/app_globals.c \
../Application/User/Core/Src/dac.c \
../Application/User/Core/Src/ili9341.c \
../Application/User/Core/Src/main.c \
../Application/User/Core/Src/rf_waveform_classifier.c \
../Application/User/Core/Src/stm32g4xx_hal_msp.c \
../Application/User/Core/Src/stm32g4xx_hal_timebase_tim.c \
../Application/User/Core/Src/stm32g4xx_it.c \
../Application/User/Core/Src/syscalls.c \
../Application/User/Core/Src/sysmem.c \
../Application/User/Core/Src/system_stm32g4xx.c \
../Application/User/Core/Src/task_current_adc.c \
../Application/User/Core/Src/task_dac.c \
../Application/User/Core/Src/task_display.c \
../Application/User/Core/Src/task_fft.c \
../Application/User/Core/Src/task_poll_button.c \
../Application/User/Core/Src/task_voltage_adc.c \
../Application/User/Core/Src/waveform_classifier.c 

OBJS += \
./Application/User/Core/Src/app_freertos.o \
./Application/User/Core/Src/app_globals.o \
./Application/User/Core/Src/dac.o \
./Application/User/Core/Src/ili9341.o \
./Application/User/Core/Src/main.o \
./Application/User/Core/Src/rf_waveform_classifier.o \
./Application/User/Core/Src/stm32g4xx_hal_msp.o \
./Application/User/Core/Src/stm32g4xx_hal_timebase_tim.o \
./Application/User/Core/Src/stm32g4xx_it.o \
./Application/User/Core/Src/syscalls.o \
./Application/User/Core/Src/sysmem.o \
./Application/User/Core/Src/system_stm32g4xx.o \
./Application/User/Core/Src/task_current_adc.o \
./Application/User/Core/Src/task_dac.o \
./Application/User/Core/Src/task_display.o \
./Application/User/Core/Src/task_fft.o \
./Application/User/Core/Src/task_poll_button.o \
./Application/User/Core/Src/task_voltage_adc.o \
./Application/User/Core/Src/waveform_classifier.o 

C_DEPS += \
./Application/User/Core/Src/app_freertos.d \
./Application/User/Core/Src/app_globals.d \
./Application/User/Core/Src/dac.d \
./Application/User/Core/Src/ili9341.d \
./Application/User/Core/Src/main.d \
./Application/User/Core/Src/rf_waveform_classifier.d \
./Application/User/Core/Src/stm32g4xx_hal_msp.d \
./Application/User/Core/Src/stm32g4xx_hal_timebase_tim.d \
./Application/User/Core/Src/stm32g4xx_it.d \
./Application/User/Core/Src/syscalls.d \
./Application/User/Core/Src/sysmem.d \
./Application/User/Core/Src/system_stm32g4xx.d \
./Application/User/Core/Src/task_current_adc.d \
./Application/User/Core/Src/task_dac.d \
./Application/User/Core/Src/task_display.d \
./Application/User/Core/Src/task_fft.d \
./Application/User/Core/Src/task_poll_button.d \
./Application/User/Core/Src/task_voltage_adc.d \
./Application/User/Core/Src/waveform_classifier.d 


# Each subdirectory must supply rules for building sources it contributes
Application/User/Core/Src/%.o Application/User/Core/Src/%.su Application/User/Core/Src/%.cyclo: ../Application/User/Core/Src/%.c Application/User/Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32G473xx -c -I../../Core/Inc -I../../USB_Device/App -I../../USB_Device/Target -I../../Drivers/STM32G4xx_HAL_Driver/Inc -I../../Drivers/STM32G4xx_HAL_Driver/Inc/Legacy -I../../Middlewares/Third_Party/FreeRTOS/Source/include -I../../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -I../../Drivers/CMSIS/Device/ST/STM32G4xx/Include -I../../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Application-2f-User-2f-Core-2f-Src

clean-Application-2f-User-2f-Core-2f-Src:
	-$(RM) ./Application/User/Core/Src/app_freertos.cyclo ./Application/User/Core/Src/app_freertos.d ./Application/User/Core/Src/app_freertos.o ./Application/User/Core/Src/app_freertos.su ./Application/User/Core/Src/app_globals.cyclo ./Application/User/Core/Src/app_globals.d ./Application/User/Core/Src/app_globals.o ./Application/User/Core/Src/app_globals.su ./Application/User/Core/Src/dac.cyclo ./Application/User/Core/Src/dac.d ./Application/User/Core/Src/dac.o ./Application/User/Core/Src/dac.su ./Application/User/Core/Src/ili9341.cyclo ./Application/User/Core/Src/ili9341.d ./Application/User/Core/Src/ili9341.o ./Application/User/Core/Src/ili9341.su ./Application/User/Core/Src/main.cyclo ./Application/User/Core/Src/main.d ./Application/User/Core/Src/main.o ./Application/User/Core/Src/main.su ./Application/User/Core/Src/rf_waveform_classifier.cyclo ./Application/User/Core/Src/rf_waveform_classifier.d ./Application/User/Core/Src/rf_waveform_classifier.o ./Application/User/Core/Src/rf_waveform_classifier.su ./Application/User/Core/Src/stm32g4xx_hal_msp.cyclo ./Application/User/Core/Src/stm32g4xx_hal_msp.d ./Application/User/Core/Src/stm32g4xx_hal_msp.o ./Application/User/Core/Src/stm32g4xx_hal_msp.su ./Application/User/Core/Src/stm32g4xx_hal_timebase_tim.cyclo ./Application/User/Core/Src/stm32g4xx_hal_timebase_tim.d ./Application/User/Core/Src/stm32g4xx_hal_timebase_tim.o ./Application/User/Core/Src/stm32g4xx_hal_timebase_tim.su ./Application/User/Core/Src/stm32g4xx_it.cyclo ./Application/User/Core/Src/stm32g4xx_it.d ./Application/User/Core/Src/stm32g4xx_it.o ./Application/User/Core/Src/stm32g4xx_it.su ./Application/User/Core/Src/syscalls.cyclo ./Application/User/Core/Src/syscalls.d ./Application/User/Core/Src/syscalls.o ./Application/User/Core/Src/syscalls.su ./Application/User/Core/Src/sysmem.cyclo ./Application/User/Core/Src/sysmem.d ./Application/User/Core/Src/sysmem.o ./Application/User/Core/Src/sysmem.su ./Application/User/Core/Src/system_stm32g4xx.cyclo ./Application/User/Core/Src/system_stm32g4xx.d ./Application/User/Core/Src/system_stm32g4xx.o ./Application/User/Core/Src/system_stm32g4xx.su ./Application/User/Core/Src/task_current_adc.cyclo ./Application/User/Core/Src/task_current_adc.d ./Application/User/Core/Src/task_current_adc.o ./Application/User/Core/Src/task_current_adc.su ./Application/User/Core/Src/task_dac.cyclo ./Application/User/Core/Src/task_dac.d ./Application/User/Core/Src/task_dac.o ./Application/User/Core/Src/task_dac.su ./Application/User/Core/Src/task_display.cyclo ./Application/User/Core/Src/task_display.d ./Application/User/Core/Src/task_display.o ./Application/User/Core/Src/task_display.su ./Application/User/Core/Src/task_fft.cyclo ./Application/User/Core/Src/task_fft.d ./Application/User/Core/Src/task_fft.o ./Application/User/Core/Src/task_fft.su ./Application/User/Core/Src/task_poll_button.cyclo ./Application/User/Core/Src/task_poll_button.d ./Application/User/Core/Src/task_poll_button.o ./Application/User/Core/Src/task_poll_button.su ./Application/User/Core/Src/task_voltage_adc.cyclo ./Application/User/Core/Src/task_voltage_adc.d ./Application/User/Core/Src/task_voltage_adc.o ./Application/User/Core/Src/task_voltage_adc.su ./Application/User/Core/Src/waveform_classifier.cyclo ./Application/User/Core/Src/waveform_classifier.d ./Application/User/Core/Src/waveform_classifier.o ./Application/User/Core/Src/waveform_classifier.su

.PHONY: clean-Application-2f-User-2f-Core-2f-Src

