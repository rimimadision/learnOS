#!/bin/bash
make all
#nasm -I include/ -o ./bin/mbr.bin ./boot/mbr.S
#nasm -I include/ -o ./bin/loader.bin ./boot/loader.S

#/usr/local/i386elfgcc/bin/i386-elf-gcc -I lib/kernel/ -I lib/ -I kernel/ -c -o build/timer.o device/timer.c
#/usr/local/i386elfgcc/bin/i386-elf-gcc -I lib/kernel/ -I lib/ -I kernel/ -c -o build/main.o kernel/main.c
#nasm -f elf -o build/print.o lib/kernel/print.S
#nasm -f elf -o build/kernel.o kernel/kernel.S
#/usr/local/i386elfgcc/bin/i386-elf-gcc -I lib/kernel/ -I lib/ -I kernel/ -c  -o build/interrupt.o kernel/interrupt.c
#/usr/local/i386elfgcc/bin/i386-elf-gcc -I lib/kernel/ -I lib/ -I kernel/ -c  -o build/init.o kernel/init.c
#/usr/local/i386elfgcc/bin/i386-elf-ld -Ttext 0xc0001500 -e main -o build/kernel.bin build/main.o build/init.o build/interrupt.o build/print.o build/kernel.o build/timer.o 
#objcopy --remove-section .eh_frame build/kernel.bin

cd /usr/local/bochs/bin/

sudo dd if=~/os/learnOS/bin/mbr.bin of=./60M.img bs=512 count=1 conv=notrunc
sudo dd if=~/os/learnOS/bin/loader.bin of=./60M.img bs=512 count=4 seek=2 conv=notrunc
#sudo dd if=~/os/learnOS/build/kernel.bin of=./60M.img bs=512 count=200 seek=9 conv=notrunc
sudo gnome-terminal -t "Bochs" -- sudo bochs -q -f bochsrc.disk
/home/linuxbrew/.linuxbrew/Cellar/i386-elf-gdb/10.2_1/bin/i386-elf-gdb-add-index ~/os/learnOS/.gdbinit
gnome-terminal -t "GDB" -- /home/linuxbrew/.linuxbrew/Cellar/i386-elf-gdb/10.2_1/bin/i386-elf-gdb ~/os/learnOS/build/kernel.bin
