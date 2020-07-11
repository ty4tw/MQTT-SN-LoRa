################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../LoRaEz/adc.c \
../LoRaEz/delay.c \
../LoRaEz/device.c \
../LoRaEz/eeprom-board.c \
../LoRaEz/eeprom.c \
../LoRaEz/fifo.c \
../LoRaEz/gpio.c \
../LoRaEz/i2c.c \
../LoRaEz/lpm.c \
../LoRaEz/nvmm.c \
../LoRaEz/rtc.c \
../LoRaEz/spi.c \
../LoRaEz/sx1276-device.c \
../LoRaEz/systime.c \
../LoRaEz/timer.c \
../LoRaEz/uart.c 

OBJS += \
./LoRaEz/adc.o \
./LoRaEz/delay.o \
./LoRaEz/device.o \
./LoRaEz/eeprom-board.o \
./LoRaEz/eeprom.o \
./LoRaEz/fifo.o \
./LoRaEz/gpio.o \
./LoRaEz/i2c.o \
./LoRaEz/lpm.o \
./LoRaEz/nvmm.o \
./LoRaEz/rtc.o \
./LoRaEz/spi.o \
./LoRaEz/sx1276-device.o \
./LoRaEz/systime.o \
./LoRaEz/timer.o \
./LoRaEz/uart.o 

C_DEPS += \
./LoRaEz/adc.d \
./LoRaEz/delay.d \
./LoRaEz/device.d \
./LoRaEz/eeprom-board.d \
./LoRaEz/eeprom.d \
./LoRaEz/fifo.d \
./LoRaEz/gpio.d \
./LoRaEz/i2c.d \
./LoRaEz/lpm.d \
./LoRaEz/nvmm.d \
./LoRaEz/rtc.d \
./LoRaEz/spi.d \
./LoRaEz/sx1276-device.d \
./LoRaEz/systime.d \
./LoRaEz/timer.d \
./LoRaEz/uart.d 


# Each subdirectory must supply rules for building sources it contributes
LoRaEz/%.o: ../LoRaEz/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m0plus -mthumb -mfloat-abi=soft -std=c11 -DABZ78 -DUSE_HAL_DRIVER -DSTM32L082xx -I"/home/tomoaki/git-MQTT-SN-LoRa/MQTT-SN-LoRa/AppSrc" -I"/home/tomoaki/git-MQTT-SN-LoRa/MQTT-SN-LoRa/LoRaEz" -I"/home/tomoaki/git-MQTT-SN-LoRa/MQTT-SN-LoRa/LoRaEz/mcu" -I"/home/tomoaki/git-MQTT-SN-LoRa/MQTT-SN-LoRa/LoRaEz/mcu/cmsis" -I"/home/tomoaki/git-MQTT-SN-LoRa/MQTT-SN-LoRa/LoRaEz/mcu/cmsis/arm-gcc" -I"/home/tomoaki/git-MQTT-SN-LoRa/MQTT-SN-LoRa/LoRaEz/mcu/STM32L0xx_HAL_Driver/Inc" -I"/home/tomoaki/git-MQTT-SN-LoRa/MQTT-SN-LoRa/LoRaEz/mcu/STM32L0xx_HAL_Driver/Inc/Legacy" -I"/home/tomoaki/git-MQTT-SN-LoRa/MQTT-SN-LoRa/LoRaEz/sx1276" -I"/home/tomoaki/git-MQTT-SN-LoRa/MQTT-SN-LoRa/LoRaLink" -I"/home/tomoaki/git-MQTT-SN-LoRa/MQTT-SN-LoRa/MQTTSN" -I"/home/tomoaki/git-MQTT-SN-LoRa/MQTT-SN-LoRa/System" -I"/home/tomoaki/git-MQTT-SN-LoRa/MQTT-SN-LoRa/System/soft-se" -O0 -g -Wall -fmessage-length=0 -ffunction-sections -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


