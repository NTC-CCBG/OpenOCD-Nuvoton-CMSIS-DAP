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

# NCT6692D targets
TARGET_6692D_FIU     = nct6692d_fiu_algo
OBJS_6692D_FIU       = nct6692d_fiu_flash.o

# NCT6694D targets (both FIU and SPIM)
TARGET_6694D_FIU     = nct6694d_fiu_algo
TARGET_6694D_SPIM    = nct6694d_spim_algo
OBJS_6694D_FIU       = nct6694d_fiu_flash.o
OBJS_6694D_SPIM      = nct6694d_spim_flash.o

FLAGS          = -mthumb -Os -ffunction-sections -fdata-sections -g -gdwarf-3 --specs=nano.specs
FLAGS         += -gstrict-dwarf -Wall -fno-strict-aliasing --asm

CFLAGS         = -c -I. -mcpu=cortex-m4 -fpack-struct

PRE_LD_FILE    = nct_esio_flash.lds
LD_FILE        = nct_esio_flash_generated.lds
LDFLAGS        = -Wl,-Map,lfw.map -Wl,-T$(LD_FILE) -nostartfiles

all: $(TARGET_6692D_FIU).inc $(TARGET_6694D_FIU).inc $(TARGET_6694D_SPIM).inc

# Implicit rules
%.o: %.c
	-@ echo CC $@ from $<
	@$(CC) $< $(FLAGS) $(CFLAGS) -o $@

$(LD_FILE): $(PRE_LD_FILE)
	-@ echo Generate $@ from $<
	-@$(CPP) $(PRE_LD_FILE) | grep -v '^#' >>$(LD_FILE)

# NCT6692D FIU target
$(TARGET_6692D_FIU).elf: $(OBJS_6692D_FIU) $(LD_FILE)
	-@ echo LD  $@ from $<
	@$(LD) -o $@ $< $(FLAGS) $(LDFLAGS)

# NCT6694D FIU target
$(TARGET_6694D_FIU).elf: $(OBJS_6694D_FIU) $(LD_FILE)
	-@ echo LD  $@ from $<
	@$(LD) -o $@ $< $(FLAGS) $(LDFLAGS)

# NCT6694D SPIM target
$(TARGET_6694D_SPIM).elf: $(OBJS_6694D_SPIM) $(LD_FILE)
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
