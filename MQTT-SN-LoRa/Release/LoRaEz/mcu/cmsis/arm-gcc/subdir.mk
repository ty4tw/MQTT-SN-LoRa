################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
S_SRCS += \
../LoRaEz/mcu/cmsis/arm-gcc/startup_stm32l082xx.s 

OBJS += \
./LoRaEz/mcu/cmsis/arm-gcc/startup_stm32l082xx.o 


# Each subdirectory must supply rules for building sources it contributes
LoRaEz/mcu/cmsis/arm-gcc/%.o: ../LoRaEz/mcu/cmsis/arm-gcc/%.s
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Assembler'
	@echo $(PWD)
	arm-none-eabi-as -mcpu=cortex-m0plus -mthumb -mfloat-abi=soft -g -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


