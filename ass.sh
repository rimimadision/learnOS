#!/bin/bash

nasm -I include/ -o ./bin/mbr.bin mbr.S
nasm -I include/ -o ./bin/loader.bin loader.S

cd /usr/local/bochs/bin/

sudo dd if=~/os/learnOS/bin/mbr.bin of=./60M.img bs=512 count=1 conv=notrunc
sudo dd if=~/os/learnOS/bin/loader.bin of=./60M.img bs=512 count=1 seek=2 conv=notrunc
sudo bochs -f bochsrc.disk
