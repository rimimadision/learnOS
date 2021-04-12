#!/bin/bash

nasm -I include/ -o ./bin/mbr.bin ./boot/mbr.S
nasm -I include/ -o ./bin/loader.bin ./boot/loader.S

# I can't get correct result using gcc64, so I use gcc32 on another linux to get kernel.bin
# The shell code is same as below 
#gcc -c -o ./kernel/main.o ./kernel/main.c
#ld kernel/main.o -Ttext 0xc0001500 -e main -o ./bin/kernel.bin 

cd /usr/local/bochs/bin/

sudo dd if=~/os/learnOS/bin/mbr.bin of=./60M.img bs=512 count=1 conv=notrunc
sudo dd if=~/os/learnOS/bin/loader.bin of=./60M.img bs=512 count=4 seek=2 conv=notrunc
sudo dd if=~/os/learnOS/bin/kernel.bin of=./60M.img bs=512 count=200 seek=9 conv=notrunc
sudo bochs -f bochsrc.disk
