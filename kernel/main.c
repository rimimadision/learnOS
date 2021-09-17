#include "print.h"
#include "init.h"
#include "thread.h"
#include "interrupt.h"
#include "console.h"
#include "ioqueue.h"
#include "keyboard.h"
#include "debug.h"
#include "process.h"
#include "syscall.h"
#include "syscall-init.h"
#include "stdio-kernel.h"
#include "stdio.h"
#include "timer.h"
#include "ide.h"
#include "fs.h"
#include "dir.h"
#include "shell.h"

int main(void){
	
	init_all();
	intr_enable();
	init_done = true;
	cls_screen();
	//console_put_str("[rabbit@localhost /]$ ");	
	while(1);
	return 0;
}

void init(void) {	
	while(!init_done);
	uint32_t ret_pid = fork();
	if (ret_pid) {
		while(1);
	} else {
		my_shell();
	}
	PANIC("init: should not be here\n");
}
