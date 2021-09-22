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

/*	uint32_t file_size = 6071; 
	uint32_t sec_cnt = DIV_ROUND_UP(file_size, 512);	
	struct disk* sda = &channels[0].devices[0];
	void* prog_buf = sys_malloc(file_size);
	ide_read(sda, 300, prog_buf, sec_cnt);
	printk("%x", *(int*)(prog_buf));
	int32_t fd = sys_open("/prog_no_arg", O_CREAT | O_RDWR);
	if (fd != -1) {
		if (sys_write(fd, prog_buf, file_size) == -1) {
			printk("file write error\n");
			while(1);
		}
	}
	sys_close(fd);
	sys_free(prog_buf);
*/	cls_screen();
	init_done = true;
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
