BUILD_DIR = ./build
ENTRY_POINT = 0Xc0001500
AS = nasm
CC = /usr/local/i386elfgcc/bin/i386-elf-gcc
LD = /usr/local/i386elfgcc/bin/i386-elf-ld
LIB = -I lib/ \
	  -I lib/kernel/ \
	  -I lib/user/ \
	  -I kernel/ \
	  -I device/
ASFLAGS = -f elf
CFLAGS = -Wall $(LIB) -c -W -Wstrict-prototypes -Wmissing-prototypes
LDFLAGS = -Ttext $(ENTRY_POINT) -e main -Map $(BUILD_DIR)/kernel.map
OBJS = $(BUILD_DIR)/main.o $(BUILD_DIR)/init.o $(BUILD_DIR)/interrupt.o \
	   $(BUILD_DIR)/timer.o $(BUILD_DIR)/kernel.o $(BUILD_DIR)/print.o \
	   $(BUILD_DIR)/debug.o
#####          C file          #####
$(BUILD_DIR)/main.o : kernel/main.c lib/kernel/print.h \
					  lib/stdint.h kernel/init.h
	$(CC) $(CFLAGS) $< -o $@
$(BUILD_DIR)/init.o : kernel/init.c kernel/init.h lib/kernel/print.h \
					  lib/stdint.h kernel/interrupt.h device/timer.h
	$(CC) $(CFLAGS) $< -o $@
$(BUILD_DIR)/interrupt.o : kernel/interrupt.c kernel/interrupt.h \
					  lib/stdint.h kernel/global.h lib/kernel/io.h lib/kernel/print.h
	$(CC) $(CFLAGS) $< -o $@
$(BUILD_DIR)/timer.o : device/timer.c device/timer.h lib/stdint.h lib/kernel/print.h \
					  lib/kernel/io.h
	$(CC) $(CFLAGS) $< -o $@
$(BUILD_DIR)/debug.o : kernel/debug.c kernel/debug.h lib/kernel/print.h \
					  lib/stdint.h kernel/interrupt.h
	$(CC) $(CFLAGS) $< -o $@

#####         asm file           #####
$(BUILD_DIR)/kernel.o : kernel/kernel.S
	$(AS) $(ASFLAGS) $< -o $@
$(BUILD_DIR)/print.o : lib/kernel/print.S
	$(AS) $(ASFLAGS) $< -o $@

#####        LD *.o              #####
$(BUILD_DIR)/kernel.bin : $(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@ \
	&& objcopy --remove-section .eh_frame $@

.PHONY : mk_dir hd clean all

mk_dir:
	if [ ! -d $(BUILD_DIR) ]; then mkdir $(BUILD_DIR); fi	
hd: 
	sudo dd if=$(BUILD_DIR)/kernel.bin of=/usr/local/bochs/bin/60M.img bs=512 count=200 seek=9 conv=notrunc

clean: 
	cd ~/os/learnOS/$(BUILD_DIR) \
	&& rm -rf ./*

build: $(BUILD_DIR)/kernel.bin

all: mk_dir build hd