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
#include "stdio.h"

void k_thread_a(void* arg);
void k_thread_b(void* arg);
void u_prog_a(void);
void u_prog_b(void);
int pid_a = 0, pid_b = 0;

int main(void){
	
	init_all();

	//process_execute(u_prog_a, "user_prog_a");
	//process_execute(u_prog_b, "user_prog_b");
	
	//console_put_str("main_pid:0x");
	//console_put_int(sys_getpid());
	//console_put_str("\n");
	
	thread_start("k_thread_a", 31, k_thread_a, "argA");
	thread_start("k_thread_b", 31, k_thread_b, "argB");
			
	intr_enable();
	while(1);
	return 0;
}

void k_thread_a(void* arg)
{
    void* addr = sys_malloc(33);
	printf("%s %x\n", (char*)arg, (int)addr);	
	while(1);
}

void k_thread_b(void* arg)
{
	void* addr = sys_malloc(63);
	printf("%s %x\n", (char*)arg, (int)addr);		
	while(1);
}

void u_prog_a() {
	char buf[20];
	sprintf(buf, "nihao%d%s\n",1, "he");
	printf("%s", buf);
	while(1);
}

void u_prog_b() {
	printf("b:%x\n", getpid());
	while(1);
}
