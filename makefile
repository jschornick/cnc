# Makefile for CNC controller

DEVICE = MSP432P401R

SRC_DIR	= src
INC_DIR	= include
LD_DIR	= link
BUILD_DIR	= BUILD
SCRIPT_DIR = script

MSP_INC_DIR   = $(INC_DIR)/ti
CMSIS_INC_DIR = $(INC_DIR)/arm
DRIVERLIB_DIR = $(HOME)/ti/msp432_driverlib

GCC_BIN_DIR = /usr/bin
CC          = $(GCC_BIN_DIR)/arm-none-eabi-gcc
GDB         = $(GCC_BIN_DIR)/arm-none-eabi-gdb -q

INCLUDES    = -I$(INC_DIR) -I$(CMSIS_INC_DIR) -I$(MSP_INC_DIR) -I$(DRIVERLIB_DIR)/driverlib/MSP432P4xx -I$(DRIVERLIB_DIR)/inc
CPP_FLAGS   = -D__$(DEVICE)__ -Dgcc
C_FLAGS     = $(CPP_FLAGS) $(INCLUDES)
C_FLAGS    += -std=c99
C_FLAGS    += -mcpu=cortex-m4 -march=armv7e-m -mfloat-abi=hard -mfpu=fpv4-sp-d16 -mthumb
C_FLAGS    += -O0
C_FLAGS    += -Wall -Werror
C_FLAGS    += -g3 -gstrict-dwarf
C_FLAGS    += -ffunction-sections -fdata-sections
#C_FLAGS    += -MD

LD_SCRIPT   = $(LD_DIR)/msp432p401r.lds
LD_FLAGS    = -mcpu=cortex-m4 -march=armv7e-m -mfloat-abi=hard -mfpu=fpv4-sp-d16 -mthumb
LD_FLAGS   += -T$(LD_SCRIPT)
LD_FLAGS   += -lc -lgcc -lnosys
LD_FLAGS   += -ffunction-sections -fdata-sections
LD_FLAGS   += -Wl,-Map=$(NAME).map
LD_FLAGS   += -lm

# If using functions/macros from TI's driverlib, we need to link the library
#LD_FLAGS   += $(DRIVERLIB_DIR)/driverlib/MSP432P4xx/gcc/msp432p4xx_driverlib.a

C_SOURCES = $(NAME).c
C_SOURCES += system_msp432p401r.c startup_msp432p401r_gcc.c
C_SOURCES += uart.c fifo.c spi.c gpio.c timer.c
C_SOURCES += tmc.c buttons.c menu.c motion.c gcode.c interpolate.c

OBJECTS   = $(addprefix $(BUILD_DIR)/, $(C_SOURCES:.c=.o))
BINARY    = $(NAME).elf

# The main project to build
NAME  = cnc

all: $(BINARY)

$(OBJECTS): | $(BUILD_DIR)

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.i: $(SRC_DIR)/%.c
	$(CC) $(C_FLAGS) -E $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(C_FLAGS) -c $< -o $@

$(BINARY): $(OBJECTS)
	@echo Linking...
	$(CC) $^ $(LD_FLAGS) -o $@
	@echo
	arm-none-eabi-size $@

.PHONY: debug
debug: $(BINARY)
	@$(SCRIPT_DIR)/debugger.sh debug $(BINARY)

.PHONY: flash
flash: $(BINARY)
	@$(SCRIPT_DIR)/debugger.sh flash $(BINARY)

clean:
	@rm -rf $(BUILD_DIR) *.elf *.pid *.log *.map
