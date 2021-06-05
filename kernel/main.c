#include "print.h"
#include "init.h"
#include "memory.h"

int main(void){
	init_all();

	void* addr = get_kernel_pages(3);
	put_str("\n alloc k_pages start from : 0x");
	put_int((uint32_t)addr);
	put_str("\n");
	//asm volatile("sti");
	while(1);
	return 0;
}
