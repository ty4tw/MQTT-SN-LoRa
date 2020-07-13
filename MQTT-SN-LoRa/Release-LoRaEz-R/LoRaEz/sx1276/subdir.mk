################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../LoRaEz/sx1276/sx1276.c 

OBJS += \
./LoRaEz/sx1276/sx1276.o 

C_DEPS += \
./LoRaEz/sx1276/sx1276.d 


# Each subdirectory must supply rules for building sources it contributes
LoRaEz/sx1276/%.o: ../LoRaEz/sx1276/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m0plus -mthumb -mfloat-abi=soft -std=c11 -DABZ78_R -DUSE_HAL_DRIVER -DSTM32L082xx -I"/home/tomoaki/git-MQTT-SN-LoRa/MQTT-SN-LoRa/AppSrc" -I"/home/tomoaki/git-MQTT-SN-LoRa/MQTT-SN-LoRa/LoRaEz" -I"/home/tomoaki/git-MQTT-SN-LoRa/MQTT-SN-LoRa/LoRaEz/mcu" -I"/home/tomoaki/git-MQTT-SN-LoRa/MQTT-SN-LoRa/LoRaEz/mcu/cmsis" -I"/home/tomoaki/git-MQTT-SN-LoRa/MQTT-SN-LoRa/LoRaEz/mcu/cmsis/arm-gcc" -I"/home/tomoaki/git-MQTT-SN-LoRa/MQTT-SN-LoRa/LoRaEz/mcu/STM32L0xx_HAL_Driver/Inc" -I"/home/tomoaki/git-MQTT-SN-LoRa/MQTT-SN-LoRa/LoRaEz/mcu/STM32L0xx_HAL_Driver/Inc/Legacy" -I"/home/tomoaki/git-MQTT-SN-LoRa/MQTT-SN-LoRa/LoRaEz/sx1276" -I"/home/tomoaki/git-MQTT-SN-LoRa/MQTT-SN-LoRa/LoRaLink" -I"/home/tomoaki/git-MQTT-SN-LoRa/MQTT-SN-LoRa/MQTTSN" -I"/home/tomoaki/git-MQTT-SN-LoRa/MQTT-SN-LoRa/System" -O3 -Wall -fmessage-length=0 -ffunction-sections -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


