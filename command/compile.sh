#!/bin/bash

if [[ ! -d "../lib" || ! -d "../build" ]]; then
	echo "dependent dir don\'t exist"
	cwd=$(pwd)
	cwd=${cwd##*/}
	cwd=${cwd%/}
	if [[ $cwd != "command" ]]; then
		echo -e "you\'d better in commmand dir\n"
	fi
	exit
fi

### In command directory ###
CC="/usr/local/i386elfgcc/bin/i386-elf-gcc"
LD="/usr/local/i386elfgcc/bin/i386-elf-ld"
AR="/usr/local/i386elfgcc/bin/i386-elf-ar"

BIN="prog_arg"
CFLAGS="-Wall -c -fno-builtin -W -Wstrict-prototypes -Wmissing-prototypes -Wsystem-headers"
LIB=" -I ../lib/ \
	  -I ../lib/kernel/ \
	  -I ../lib/user/ \
	  -I ../kernel/ \
	  -I ../device/ \
	  -I ../thread/ \
	  -I ../userprog/ \
	  -I ../fs/\
	  -I ../shell/"
OBJS="../build/string.o ../build/syscall.o ../build/stdio.o ../build/assert.o start.o"
DD_IN=$BIN
DD_OUT="/usr/local/bochs/bin/60M.img"

nasm -f elf ./start.S -o ./start.o
$CC $CFLAGS $LIB -o ../build/assert.o ../lib/assert.c
$AR rcs simple_crt.a $OBJS
$CC $CFLAGS $LIB -o $BIN.o $BIN.c
$LD $BIN.o simple_crt.a -o $BIN
SEC_CNT=$(ls -l $BIN|awk '{printf("%d", ($5+511)/512)}')

if [[ -f $BIN ]]; then
	dd if=./$DD_IN of=$DD_OUT bs=512 count=$SEC_CNT seek=300 conv=notrunc
fi
