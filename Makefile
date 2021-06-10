MODULE := ABZ78

PROGRAM := Firmware

CCPATH :=

CCARM :=$(CCPATH)arm-none-eabi-gcc
CXXARM :=$(CCPATH)arm-none-eabi-g++
ASARM := $(CCPATH)arm-none-eabi-as
ARARM := $(CCPATH)arm-none-eabi-ar
CONV := $(CCPATH)arm-none-eabi-objcopy
SIZE := $(CCPATH)arm-none-eabi-size
FLASH := $(CCPATH)/../../STM32CubeProgrammer/bin/STM32_Programmer.sh
RM := rm 

PORT := /dev/ttyUSB0

TYPE :=

SRCDIR := AppSrc
LORAEZ := LoRaEz
MCU    := $(LORAEZ)/mcu
SX1276 := $(LORAEZ)/sx1276
CMSIS  := $(MCU)/cmsis
ARMGCC := $(CMSIS)/arm-gcc
HALDRIVER := $(MCU)/STM32L0xx_HAL_Driver/Inc
LEGACY := $(HALDRIVER)/Legacy
LORALINK   := LoRaLink
MQTTSN := MQTTSN
SYSTEM := System

LOG :=

INCLUDE :=
INCLUDES += $(INCLUDE) \
-I$(SRCDIR) \
-I$(LORAEZ) \
-I$(MCU) \
-I$(CMSIS) \
-I$(ARMGCC) \
-I$(HALDRIVER) \
-I$(LEGACY) \
-I$(SX1276) \
-I$(LORALINK) \
-I$(MQTTSN) \
-I$(SYSTEM) 


APPCSRCS :=     ${shell find $(SRCDIR) -name *.c }
LORAEZSRCS :=   ${shell find $(LORAEZ) -name *.c }
LORALINKSRCS := ${shell find $(LORALINK) -name *.c }
MQTTSNSRCS :=   ${shell find $(MQTTSN) -name *.c }
SYSTEMSRCS :=   ${shell find $(SYSTEM) -name *.c }


ASRCS := ${shell find $(ARMGCC) -name startup_*.s }

LD := ${shell find $(ARMGCC) -name  stm32*.ld }

STM82 := STM32L082xx

DEFS := -DUSE_HAL_DRIVER

LIB :=
LIBS += $(LIB) -s -static  
MCUFLAGS := -mcpu=cortex-m0plus  -mthumb -mfloat-abi=soft
LDFLAGS := -specs=nosys.specs -specs=nano.specs -u _printf_float -Wl,--gc-sections -fno-rtti -s -fno-exceptions

CCFLAGS := -O3 -Wall -fmessage-length=0 -fno-strict-aliasing  -ffunction-sections -fdata-sections -fno-exceptions 

LDDIR := 
LDADD := 
LDADD += -lm

OUTDIR := Build
PROG := $(OUTDIR)/$(PROGRAM)

APPCOBJS :=      $(APPCSRCS:%.c=$(OUTDIR)/%.o)
LORAEZOBJS :=    $(LORAEZSRCS:%.c=$(OUTDIR)/%.o)
LORALINKOBJSS := $(LORALINKSRCS:%.c=$(OUTDIR)/%.o)
MQTTSNOBJS :=    $(MQTTSNSRCS:%.c=$(OUTDIR)/%.o)
SYSTEMOBJS :=    $(SYSTEMSRCS:%.c=$(OUTDIR)/%.o)

ASOBJS := $(ASRCS:%.s=$(OUTDIR)/%.o)

DEPS := $(APPCSRCS:%.c=$(OUTDIR)/%.d)
DEPS += $(LORAEZSRCS:%.c=$(OUTDIR)/%.d)
DEPS += $(LORALINKSRCS:%.c=$(OUTDIR)/%.d)
DEPS += $(MQTTSNSRCS:%.c=$(OUTDIR)/%.d)
DEPS += $(SYSTEMSRCS:%.c=$(OUTDIR)/%.d)


.PHONY: flash clean

all: $(PROG) 


$(PROG):$(APPCOBJS) $(LORAEZOBJS) $(LORALINKOBJSS) $(MQTTSNOBJS) $(SYSTEMOBJS) $(ASOBJS)
	$(CCARM) $(MCUFLAGS) $(LDFLAGS) $(LDDIR) $(LIBS) -T$(LD) -o $(PROG).elf $(APPCOBJS) $(LORAEZOBJS) $(LORALINKOBJSS) $(MQTTSNOBJS) $(SYSTEMOBJS) $(ASOBJS) $(LDADD) 
	$(CONV) -O binary $(PROG).elf $(PROG).bin
	$(SIZE) -B $(PROG).elf

$(OUTDIR)/$(SRCDIR)/%.o : $(SRCDIR)/%.c
	@if [ ! -e `dirname $@` ]; then mkdir -p `dirname $@`; fi
	$(CCARM) -D$(STM82) -D$(MODULE) -D$(TYPE) -D$(LOG) $(MCUFLAGS) $(DEFS) $(INCLUDES) $(CCFLAGS) -std=c11  -o $@ -c $<  -MMD -MP -MF $(@:%.o=%.d) 	

	
$(OUTDIR)/$(LORAEZ)/%.o : $(LORAEZ)/%.c
	@if [ ! -e `dirname $@` ]; then mkdir -p `dirname $@`; fi
	$(CCARM) -D$(STM82) -D$(MODULE) -D$(LOG) $(MCUFLAGS) $(DEFS) $(INCLUDES) $(CCFLAGS) -std=c11  -o $@ -c $<  -MMD -MP -MF $(@:%.o=%.d) 


$(OUTDIR)/$(LORALINK)/%.o : $(LORALINK)/%.c
	@if [ ! -e `dirname $@` ]; then mkdir -p `dirname $@`; fi
	$(CCARM) -D$(STM82) -D$(MODULE)  -D$(LOG) $(MCUFLAGS) $(DEFS) $(INCLUDES) $(CCFLAGS) -std=c11  -o $@ -c $<  -MMD -MP -MF $(@:%.o=%.d) 
	
	
$(OUTDIR)/$(MQTTSN)/%.o : $(MQTTSN)/%.c
	@if [ ! -e `dirname $@` ]; then mkdir -p `dirname $@`; fi
	$(CCARM) -D$(STM82) -D$(MODULE) -D$(LOG) $(MCUFLAGS) $(DEFS) $(INCLUDES) $(CCFLAGS) -std=c11  -o $@ -c $<  -MMD -MP -MF $(@:%.o=%.d) 
	
		
$(OUTDIR)/$(SYSTEM)/%.o : $(SYSTEM)/%.c
	@if [ ! -e `dirname $@` ]; then mkdir -p `dirname $@`; fi
	$(CCARM) -D$(STM82) -D$(MODULE) -D$(LOG) $(MCUFLAGS) $(DEFS) $(INCLUDES) $(CCFLAGS) -std=c11  -o $@ -c $<  -MMD -MP -MF $(@:%.o=%.d) 	

$(OUTDIR)/$(ARMGCC)/%.o : $(ARMGCC)/%.s
	@if [ ! -e `dirname $@` ]; then mkdir -p `dirname $@`; fi
		@if [ ! -e `dirname $@` ]; then mkdir -p `dirname $@`; fi
	$(ASARM) $(MCUFLAGS)  -o $@ -c   $<	

clean:
	$(RM) -rf $(OUTDIR)
	

flash:
	$(FLASH)  port=$(PORT) br=500000 -w $(PROG).bin  0x08000000
	