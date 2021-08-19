#include "syscall-init.h"
#include "stdint.h"
#include "thread.h"
#include "print.h"
#include "syscall.h"

#define syscall_nr 32
typedef void* syscall;
syscall syscall_table[syscall_nr];

uint32_t sys_getpid(void) {
	return get_cur_thread_pcb()->pid; 
}

void syscall_init(void) {
	put_str("syscall_init start\n");
	syscall_table[SYS_GETPID] = sys_getpid;
	put_str("syscall_init done\n");
}
