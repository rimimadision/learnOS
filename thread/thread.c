#include "thread.h"
#include "stdint.h"
#include "string.h"
#include "global.h"
#include "memory.h"

#define PG_SIZE 4096


static void kernel_thread(thread_func* function, void* func_arg);
void thread_create(struct task_struct* pthread, thread_func function, void* func_arg);
void init_thread(struct task_struct* pthread, char* name, int prio);
struct task_struct* thread_start(char* name, int prio, thread_func function, void* func_arg);

/* execute the thread_func through this function */
static void kernel_thread(thread_func* function, void* func_arg)
{
	function(func_arg);
}

/* initialize thread_stack */
void thread_create(struct task_struct* pthread, thread_func function, void* func_arg)
{
	/* reserve space for intr_stack */
	pthread->self_kstack -= sizeof(struct intr_stack);
	
	/* make self_kstack points to thread_stack */
	pthread->self_kstack -= sizeof(struct thread_stack);
	struct thread_stack* p_thread_stack = (struct thread_stack*)(pthread->self_kstack);
	p_thread_stack->ebp = 0;
	p_thread_stack->ebx = 0;
	p_thread_stack->edi = 0;
	p_thread_stack->esi = 0;
	p_thread_stack->eip = kernel_thread;
	p_thread_stack->function = function;
	p_thread_stack->func_arg = func_arg;
}

/* add information of thread into task_struct */
void init_thread(struct task_struct* pthread, char* name, int prio)
{
	memset(pthread, 0, sizeof(*pthread));
	strcpy(pthread->name, name);
	pthread->status = TASK_RUNNING;
	pthread->priority = prio;
	pthread->self_kstack = (uint32_t*)((uint32_t)pthread + PG_SIZE);
	pthread->stack_magic = 0x19700505;
}

/* API to start a new thread */
struct task_struct* thread_start(char* name, int prio, thread_func function, void* func_arg)
{
	struct task_struct* thread = get_kernel_pages(1); // PCB starts from the bottom of page
	
	init_thread(thread, name, prio);
	thread_create(thread, function, func_arg);

	asm volatile ("movl %0, %%esp;\
					pop %%ebp;\
					pop %%ebx;\
					pop %%edi;\
					pop %%esi;\
					ret"\
					: : "g"(thread->self_kstack) : "memory");
	
	return thread; // will not be excuted
}

