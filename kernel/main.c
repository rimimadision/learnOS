#include "print.h"
#include "init.h"
#include "thread.h"

void k_thread_a(void* arg);

int main(void){
	init_all();

	thread_start("k_thread_a", 31, k_thread_a, "argA ");
	//asm volatile("sti");
	while(1);
	return 0;
}

void k_thread_a(void* arg)
{
	char* para = arg;
	while(1)
	{	
		put_str(para);
	}
}
