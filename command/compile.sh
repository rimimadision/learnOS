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
BIN="prog_no_arg"
CFLAGS="-Wall -c -fno-builtin -W -Wstrict-prototypes -Wmissing-prototypes -Wsystem-headers"
LIB="../lib/"
OBJS="../build/string.o ../build/syscall.o ../build/stdio.o ../build/assert.o"
DD_IN=$BIN
DD_OUT="/usr/local/bochs/bin/60M.img"
$CC $CFLAGS -I $LIB -o $BIN.o $BIN.c
$LD 
