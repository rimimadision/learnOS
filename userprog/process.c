#include "thread.h"
#include "global.h"
#include "memory.h"
#include "userprog.h"

extern void intr_exit(void);

void start_process(void* filename;) {
	void* function = filename_;
	struct task_struct* cur = get_cur_thread_pcb();
	cur->self_kstack += sizeof(struct thread_stack);
	struct intr_stack* proc_stack = (struct intr_stack*)cur->self_kstack;
	proc_stack->edi = 0;
	proc_stack->esi = 0;
	proc_stack->ebp = 0;
	proc_stack->esp_dummy = 0;
	proc_stack->ebx = 0;
	proc_stack->edx = 0;
	proc_stack->ecx = 0;
	proc_stack->eax = 0;
	proc_stack->gs = 0;
	proc_stack->fs = SELECTOR_U_DATA;
	proc_stack->es = SELECTOR_U_DATA;
	proc_stack->ds = SELECTOR_U_DATA;
	proc_stack->err_code = 0;
	proc_stack->eip = function;
	proc_stack->cs = SELECTOR_U_CODE;
	proc_stack->eflags = (EFLAGS_IOPL_0 | EFLAGS_MBS | EFLAGS_IF_1);
	proc_stack->esp = (void*)((uint32_t)get_a_page(PF_USER, USER_STACK3_VADDR) + PG_SIZE); 
	proc_stack->ss = SELECTOR_U_STACK;	
	asm volatile ("movl %0, %%esp;
				   jmp intr_exit"
				   : 
				   : "g"(proc_stack)
				   : "memory");
}

void page_dir_activate(struct task_struct* p_thread) {
	uint32_t page
}
