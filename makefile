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
	  -I thread/ \
	  -I userprog/ \
	  -I fs/\
	  -I shell/
ASFLAGS = -f elf
CFLAGS = -Wall $(LIB) -c -fno-builtin -W -Wstrict-prototypes -Wmissing-prototypes
LDFLAGS = -Ttext $(ENTRY_POINT) -e main -Map $(BUILD_DIR)/kernel.map
OBJS = $(BUILD_DIR)/main.o         $(BUILD_DIR)/init.o      $(BUILD_DIR)/shell.o\
       $(BUILD_DIR)/buildin_cmd.o  $(BUILD_DIR)/stdio.o     $(BUILD_DIR)/exec.o\
       $(BUILD_DIR)/fork.o         $(BUILD_DIR)/wait_exit.o\
	   $(BUILD_DIR)/syscall-init.o $(BUILD_DIR)/syscall.o   $(BUILD_DIR)/fs.o\
	   $(BUILD_DIR)/file.o         $(BUILD_DIR)/inode.o     $(BUILD_DIR)/dir.o\
       $(BUILD_DIR)/stdio-kernel.o $(BUILD_DIR)/ide.o\
	   $(BUILD_DIR)/keyboard.o     $(BUILD_DIR)/ioqueue.o   $(BUILD_DIR)/process.o\
	   $(BUILD_DIR)/console.o  	   $(BUILD_DIR)/tss.o\
	   $(BUILD_DIR)/sync.o         $(BUILD_DIR)/thread.o    $(BUILD_DIR)/list.o\
	   $(BUILD_DIR)/timer.o        $(BUILD_DIR)/interrupt.o $(BUILD_DIR)/kernel.o\
	   $(BUILD_DIR)/print.o        $(BUILD_DIR)/memory.o    $(BUILD_DIR)/bitmap.o\
	   $(BUILD_DIR)/string.o       $(BUILD_DIR)/debug.o     $(BUILD_DIR)/switch.o
 
#####          C file          #####
$(BUILD_DIR)/main.o : kernel/main.c lib/kernel/print.h \
					  lib/stdint.h kernel/init.h kernel/debug.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/init.o : kernel/init.c kernel/init.h lib/kernel/print.h userprog/tss.h\
					  lib/stdint.h kernel/interrupt.h device/timer.h device/console.h userprog/syscall-init.h device/ide.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/shell.o : shell/shell.c shell/shell.h lib/stdio.h kernel/global.h\
                       lib/stdint.h lib/string.h fs/file.h kernel/debug.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/buildin_cmd.o : shell/buildin_cmd.c shell/buildin_cmd.h lib/stdio.h kernel/global.h\
                       lib/stdint.h lib/string.h fs/file.h kernel/debug.h fs/dir.h\
                       shell/shell.h fs/fs.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/stdio.o : lib/stdio.c lib/stdio.h kernel/global.h lib/user/syscall.h lib/stdint.h\
					   lib/string.h 
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/exec.o : userprog/exec.c userprog/exec.h fs/fs.h lib/stdint.h\
				      lib/kernel/stdio-kernel.h kernel/memory.h userprog/process.h\
				      kernel/global.h kernel/debug.h thread/thread.h 
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/fork.o : userprog/fork.c userprog/fork.h fs/fs.h lib/stdint.h\
				      lib/kernel/stdio-kernel.h kernel/memory.h userprog/process.h\
				      lib/kernel/bitmap.h kernel/debug.h thread/thread.h 
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/wait_exit.o : userprog/wait_exit.c userprog/wait_exit.h lib/stdint.h\
				      lib/kernel/stdio-kernel.h kernel/memory.h userprog/process.h\
				      lib/kernel/bitmap.h thread/thread.h kernel/global.h fs/fs.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/syscall.o : lib/user/syscall.c lib/user/syscall.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/syscall-init.o : userprog/syscall-init.c userprog/syscall-init.h lib/stdint.h\
							  thread/thread.h lib/kernel/print.h lib/user/syscall.h\
							  device/console.h lib/string.h kernel/memory.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/fs.o : fs/fs.c fs/fs.h lib/stdint.h kernel/global.h fs/inode.h device/ide.h\
				    fs/super_block.h fs/dir.h lib/kernel/stdio-kernel.h kernel/memory.h\
				    lib/string.h lib/kernel/list.h kernel/debug.h fs/file.h\
		            thread/thread.h device/console.h 
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/file.o : fs/file.c fs/file.h lib/stdint.h kernel/global.h fs/inode.h device/ide.h\
				      fs/super_block.h fs/dir.h lib/kernel/stdio-kernel.h kernel/memory.h\
				      lib/string.h lib/kernel/list.h kernel/debug.h fs/file.h\
				      lib/kernel/bitmap.h kernel/interrupt.h thread/thread.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/inode.o : fs/inode.c fs/inode.h lib/stdint.h kernel/global.h fs/inode.h device/ide.h\
				      fs/super_block.h fs/dir.h lib/kernel/stdio-kernel.h kernel/memory.h\
				      lib/string.h lib/kernel/list.h kernel/debug.h fs/file.h\
				      lib/kernel/bitmap.h kernel/interrupt.h thread/thread.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/dir.o : fs/dir.c fs/dir.h fs/fs.h lib/stdint.h kernel/global.h fs/inode.h device/ide.h\
				      fs/super_block.h lib/kernel/stdio-kernel.h kernel/memory.h\
				      kernel/interrupt.h thread/thread.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/stdio-kernel.o : lib/kernel/stdio-kernel.c lib/kernel/stdio-kernel.h lib/stdio.h\
							  kernel/global.h device/console.h lib/stdint.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/ide.o : device/ide.c device/ide.h lib/kernel/stdio-kernel.h lib/stdio.h\
				     kernel/global.h lib/stdint.h lib/kernel/io.h device/timer.h\
					 thread/sync.h kernel/interrupt.h lib/string.h lib/kernel/list.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/keyboard.o : device/keyboard.c device/keyboard.h lib/kernel/print.h\
						 kernel/interrupt.h lib/kernel/io.h kernel/global.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/ioqueue.o : device/ioqueue.c device/ioqueue.h kernel/debug.h\
						 kernel/interrupt.h kernel/global.h 
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/process.o : userprog/process.c userprog/process.h thread/thread.h kernel/debug.h\
						 kernel/interrupt.h kernel/global.h kernel/memory.h userprog/userprog.h\
						 userprog/tss.h device/console.h lib/string.h lib/kernel/bitmap.h\
						 lib/kernel/list.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/console.o : device/console.c device/console.h lib/kernel/print.h\
						 lib/stdint.h thread/sync.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/tss.o : userprog/tss.c userprog/tss.h kernel/global.h lib/stdint.h\
					 thread/thread.h lib/kernel/print.h lib/string.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/sync.o : thread/sync.c thread/sync.h kernel/interrupt.h\
					  lib/stdint.h thread/thread.h lib/kernel/list.h kernel/debug.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/thread.o : thread/thread.c thread/thread.h lib/string.h thread/switch.S \
					  lib/stdint.h kernel/memory.h kernel/global.h lib/kernel/list.h\
					  kernel/interrupt.h thread/sync.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/list.o : lib/kernel/list.c lib/kernel/list.h kernel/interrupt.h  \
					  lib/stdint.h kernel/global.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/timer.o : device/timer.c device/timer.h lib/kernel/print.h lib/stdint.h\
					  lib/kernel/io.h kernel/interrupt.h thread/thread.h kernel/debug.h\
				      kernel/global.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/interrupt.o : kernel/interrupt.c kernel/interrupt.h kernel/kernel.S kernel/debug.h\
					  lib/stdint.h kernel/global.h lib/kernel/io.h lib/kernel/print.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/memory.o : kernel/memory.c kernel/memory.h lib/kernel/print.h kernel/global.h \
					  lib/stdint.h lib/kernel/bitmap.h kernel/debug.h lib/string.h thread/sync.h\
					  thread/thread.h kernel/interrupt.h
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
