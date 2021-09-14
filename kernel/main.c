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

void k_thread_a(void* arg);
void k_thread_b(void* arg);
void u_prog_a(void);
void u_prog_b(void);
int pid_a = 0, pid_b = 0;

int main(void){
	
	init_all();

	//process_execute(u_prog_a, "user_prog_a");
	//process_execute(u_prog_b, "user_prog_b");
	
	//thread_start("k_thread_a", 31, k_thread_a, "argA");
	//thread_start("k_thread_b", 31, k_thread_b, "argB");
	intr_enable();
	//printk("/dir1/subdir1 create %s \n",sys_mkdir("/dir1/subdir1") == 0 ? "done" : "fail");
	//printk("/dir1 create %s \n",sys_mkdir("/dir1") == 0 ? "done" : "fail");
	//printk("/dir1/subdir1 create %s \n",sys_mkdir("/dir1/subdir1") == 0 ? "done" : "fail");
	
	int fd = sys_open("/dir1/subdir1/file2", O_RDWR);
	sys_lseek(fd, 0, SEEK_SET);
	char buf[32] = {0};
	sys_read(fd, buf, 10);
	printk("/dir1/subdir1/file2 says:\n%s\n", buf);
	sys_close(fd);
	
	
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
	
	int max = 1;
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
		
	printk("%s\n", (char*)arg);	
	while(1);
}

void k_thread_b(void* arg)
{
	void* addr1;
	void* addr2;
	void* addr3;
	void* addr4;
	void* addr5;
	
	int max = 1;
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
	void* addr1 = malloc(256);
	void* addr2 = malloc(255);
	void* addr3 = malloc(254);

	printf("prog_a : 0x%x, 0x%x, 0x%x\n", (int)addr1, (int)addr2, (int)addr3);

	free(addr1);
	free(addr2);
	free(addr3);	

	while(1);
}

void u_prog_b() {
	void* addr1 = malloc(256);
	void* addr2 = malloc(255);
	void* addr3 = malloc(254);

	printf("prog_a : 0x%x, 0x%x, 0x%x\n", (int)addr1, (int)addr2, (int)addr3);

	free(addr1);
	free(addr2);
	free(addr3);	
	
	while(1);
}
