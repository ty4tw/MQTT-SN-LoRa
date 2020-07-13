################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include System/subdir.mk
-include MQTTSN/subdir.mk
-include LoRaLink/subdir.mk
-include LoRaEz/sx1276/subdir.mk
-include LoRaEz/mcu/cmsis/arm-gcc/subdir.mk
-include LoRaEz/mcu/cmsis/subdir.mk
-include LoRaEz/mcu/STM32L0xx_HAL_Driver/Src/subdir.mk
-include LoRaEz/subdir.mk
-include AppSrc/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: MQTT-SN-LoRa.elf

# Tool invocations
MQTT-SN-LoRa.elf: $(OBJS) $(USER_OBJS) /home/tomoaki/git-MQTT-SN-LoRa/MQTT-SN-LoRa/LoRaEz/mcu/cmsis/arm-gcc/stm32l082xx_flash.ld
	@echo 'Building target: $@'
	@echo 'Invoking: MCU GCC Linker'
	arm-none-eabi-gcc -mcpu=cortex-m0plus -mthumb -mfloat-abi=soft -static -specs=nosys.specs -specs=nano.specs -u _printf_float -T"/home/tomoaki/git-MQTT-SN-LoRa/MQTT-SN-LoRa/LoRaEz/mcu/cmsis/arm-gcc/stm32l082xx_flash.ld" -Wl,-Map=output.map -Wl,--gc-sections -o "MQTT-SN-LoRa.elf" @"objects.list" $(USER_OBJS) $(LIBS) -lm
	@echo 'Finished building target: $@'
	@echo ' '
	$(MAKE) --no-print-directory post-build

# Other Targets
clean:
	-$(RM) *
	-@echo ' '

post-build:
	-@echo 'Generating binary and Printing size information:'
	arm-none-eabi-objcopy -O binary "MQTT-SN-LoRa.elf" "MQTT-SN-LoRa.bin"
	arm-none-eabi-size "MQTT-SN-LoRa.elf"
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY: post-build

-include ../makefile.targets
