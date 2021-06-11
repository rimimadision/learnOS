BUILD_DIR = ./build
ENTRY_POINT = 0Xc0001500
AS = nasm
CC = /usr/local/i386elfgcc/bin/i386-elf-gcc
LD = /usr/local/i386elfgcc/bin/i386-elf-ld
LIB = -I lib/ \
	  -I lib/kernel/ \
	  -I lib/user/ \
	  -I kernel/ \
	  -I device/ \
	  -I thread/
ASFLAGS = -f elf
CFLAGS = -Wall $(LIB) -c -fno-builtin -W -Wstrict-prototypes -Wmissing-prototypes
LDFLAGS = -Ttext $(ENTRY_POINT) -e main -Map $(BUILD_DIR)/kernel.map
OBJS = $(BUILD_DIR)/main.o $(BUILD_DIR)/init.o $(BUILD_DIR)/thread.o $(BUILD_DIR)/list.o\
	   $(BUILD_DIR)/timer.o $(BUILD_DIR)/interrupt.o $(BUILD_DIR)/kernel.o \
	   $(BUILD_DIR)/print.o $(BUILD_DIR)/memory.o $(BUILD_DIR)/bitmap.o \
	   $(BUILD_DIR)/string.o $(BUILD_DIR)/debug.o $(BUILD_DIR)/switch.o 
#####          C file          #####
$(BUILD_DIR)/main.o : kernel/main.c lib/kernel/print.h \
					  lib/stdint.h kernel/init.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/init.o : kernel/init.c kernel/init.h lib/kernel/print.h \
					  lib/stdint.h kernel/interrupt.h device/timer.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/thread.o : thread/thread.c thread/thread.h lib/string.h thread/switch.S \
					  lib/stdint.h kernel/memory.h kernel/global.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/list.o : lib/kernel/list.c lib/kernel/list.h kernel/interrupt.h  \
					  lib/stdint.h kernel/global.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/timer.o : device/timer.c device/timer.h lib/kernel/print.h lib/stdint.h\
					  lib/kernel/io.h kernel/interrupt.h thread/thread.h kernel/debug.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/interrupt.o : kernel/interrupt.c kernel/interrupt.h kernel/kernel.S kernel/debug.h\
					  lib/stdint.h kernel/global.h lib/kernel/io.h lib/kernel/print.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/memory.o : kernel/memory.c kernel/memory.h lib/kernel/print.h kernel/global.h \
					  lib/stdint.h lib/kernel/bitmap.h kernel/debug.h lib/string.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/bitmap.o : lib/kernel/bitmap.c kernel/debug.h lib/kernel/print.h \
					  lib/stdint.h kernel/interrupt.h lib/string.h kernel/global.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/string.o : lib/string.c kernel/debug.h lib/string.h kernel/global.h
	$(CC) $(CFLAGS) $< -o $@


$(BUILD_DIR)/debug.o : kernel/debug.c kernel/debug.h lib/kernel/print.h \
					  lib/stdint.h kernel/interrupt.h
	$(CC) $(CFLAGS) $< -o $@


#####         asm file           #####
$(BUILD_DIR)/kernel.o : kernel/kernel.S
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/print.o : lib/kernel/print.S
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/switch.o : thread/switch.S
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
