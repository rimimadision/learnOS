#!/bin/bash

nasm -o bin/mbr.bin mbr.S
cd /usr/local/bochs/bin/
sudo dd if=~/os/learnOS/bin/mbr.bin of=./60M.img bs=512 count=1 conv=notrunc
sudo bochs -f bochsrc.disk
