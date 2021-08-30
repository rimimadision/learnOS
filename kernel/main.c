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
	
	thread_start("k_thread_a", 31, k_thread_a, "argA");
	thread_start("k_thread_b", 31, k_thread_b, "argB");
			
	intr_enable();
	while(1);
	return 0;
}

void k_thread_a(void* arg)
{
	void* addr1;
	void* addr2;
	void* addr3;
	void* addr4;
	void* addr5;
	
	int max = 1000;
	while(max --) {
		int size = 128;
		addr1 = sys_malloc(size);
		size *= 2;		
		addr2 = sys_malloc(size);
		size *= 2;
		addr3 = sys_malloc(size);
		size *= 128;
		addr4 = sys_malloc(size);
		sys_free(addr1);
		sys_free(addr2);
		sys_free(addr3);
		sys_free(addr4);
	}
		
	printf("%s\n", (char*)arg);	
	while(1);
}

void k_thread_b(void* arg)
{
	void* addr1;
	void* addr2;
	void* addr3;
	void* addr4;
	void* addr5;
	
	int max = 1000;
	while(max --) {
		int size = 128;
		addr1 = sys_malloc(size);
		size *= 2;		
		addr2 = sys_malloc(size);
		size *= 2;
		addr3 = sys_malloc(size);
		size *= 16;
		addr4 = sys_malloc(size);
		sys_free(addr1);
		sys_free(addr2);
		sys_free(addr3);
		sys_free(addr4);
	}
		
	printf("%s\n", (char*)arg);	
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
