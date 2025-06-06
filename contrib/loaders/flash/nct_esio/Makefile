# SPDX-License-Identifier: GPL-2.0-or-later

BIN2C = ../../../../src/helper/bin2char.sh

# Toolchain used in makefile
CROSS_COMPILE ?= arm-none-eabi-
CC             = $(CROSS_COMPILE)gcc
CPLUS          = $(CROSS_COMPILE)g++
CPP            = $(CROSS_COMPILE)cpp
LD             = $(CROSS_COMPILE)gcc
AS             = $(CROSS_COMPILE)as
OBJCOPY        = $(CROSS_COMPILE)objcopy
OBJDUMP        = $(CROSS_COMPILE)objdump
OBJSIZE        = $(CROSS_COMPILE)size

TARGET_SPIM    = nct_esio_spim_algo
TARGET_FIU     = nct_esio_fiu_algo
OBJS_SPIM      = nct_esio_spim_flash.o
OBJS_FIU       = nct_esio_fiu_flash.o
FLAGS          = -mthumb -Os -ffunction-sections -fdata-sections -g -gdwarf-3 --specs=nano.specs
FLAGS         += -gstrict-dwarf -Wall -fno-strict-aliasing --asm

CFLAGS         = -c -I. -mcpu=cortex-m4 -fpack-struct

PRE_LD_FILE    = nct_esio_flash.lds
LD_FILE        = nct_esio_flash_generated.lds
LDFLAGS        = -Wl,-Map,lfw.map -Wl,-T$(LD_FILE) -nostartfiles

all: $(TARGET_SPIM).inc $(TARGET_FIU).inc

# Implicit rules
%.o: %.c
	-@ echo CC $@ from $<
	@$(CC) $< $(FLAGS) $(CFLAGS) -o $@

$(LD_FILE): $(PRE_LD_FILE)
	-@ echo Generate $@ from $<
	-@$(CPP) $(PRE_LD_FILE) | grep -v '^#' >>$(LD_FILE)

$(TARGET_SPIM).elf: $(OBJS_SPIM) $(LD_FILE)
	-@ echo LD  $@ from $<
	@$(LD) -o $@ $< $(FLAGS) $(LDFLAGS)

$(TARGET_FIU).elf: $(OBJS_FIU) $(LD_FILE)
	-@ echo LD  $@ from $<
	@$(LD) -o $@ $< $(FLAGS) $(LDFLAGS)

%.bin: %.elf
	-@ echo OBJCOPY $@ from $<
	-@ $(OBJCOPY) $< -O binary $@
	-@ $(OBJSIZE) $< --format=berkeley

%.inc: %.bin
	@echo 'Building target: $@'
	@echo 'Invoking Bin2Char Script'
	$(BIN2C) < $< > $@
	rm $< $*.elf
	@echo 'Finished building target: $@'
	@echo ' '

clean:
	@echo 'Cleaning Targets and Build Artifacts'
	rm -rf *.inc *.bin *.elf *.map
	rm -rf *.o *.d
	rm -rf $(LD_FILE)
	@echo 'Finished clean'
	@echo ' '

.PRECIOUS: %.bin

.PHONY: all clean
