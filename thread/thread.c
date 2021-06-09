#include "thread.h"
#include "stdint.h"
#include "string.h"
#include "global.h"
#include "memory.h"
#include "list.h"

#define PG_SIZE 4096

struct task_struct* main_thread;
struct list thread_ready_list;
struct list thread_all_list;
static struct list_elem* thread_tag; // used in converting tag in list to PCB

extern void switch_to(struct task_struct* cur, struct task_struct* next);

static void kernel_thread(thread_func* function, void* func_arg);
static void make_main_thread(void);

void thread_create(struct task_struct* pthread, thread_func* function, void* func_arg);
void init_thread(struct task_struct* pthread, char* name, int prio);

/* get current thread PCB */
struct task_struct* get_cur_thread_pcb()
{	
	/* for kernel_stack of thread is in the same page of PCB
	   and PCB is set at the bottom of page
	   so addr of PCB is esp & 0xfffff000 
	*/
	uint32_t esp;
	asm volatile("movl %%esp, %0" : "=g"(esp));
	return (struct task_struct*)(esp & 0xfffff000);
}

/* execute the thread_func through this function */
static void kernel_thread(thread_func* function, void* func_arg)
{
	intr_enable();
	function(func_arg);
}

/* initialize thread_stack */
void thread_create(struct task_struct* pthread, thread_func* function, void* func_arg)
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

	if(pthread == main_thread)
	{
		pthread->status = TASK_RUNNING;
	}else
	{
		pthread->status = TASK_READY;
	}

	pthread->priority = prio;
	pthread->ticks = prio;
	pthread->elapsed_ticks = 0;
	pthread->pgdir = NULL;
	pthread->self_kstack = (uint32_t*)((uint32_t)pthread + PG_SIZE);
	pthread->stack_magic = 0x19700505;
}

/* API to start a new thread */
struct task_struct* thread_start(char* name, int prio, thread_func* function, void* func_arg)
{
	struct task_struct* thread = get_kernel_pages(1); // PCB starts from the bottom of page
	
	init_thread(thread, name, prio);
	thread_create(thread, function, func_arg);

	ASSERT(!elem_find(&thread_ready_list, &thread->general_tag));
	list_append(&thread_ready_list, &thread->general_tag);

	ASSERT(!elem_find(&thread_all_list, &thread->all_list_tag));
	list_append(&thread_all_list, &thread->all_list_tag);

	return thread;
}

static void make_main_thread(void)
{
	/* already leaving 0xc009e000 ~ 0xc009efff as PCB */
	main_thread = get_cur_thread_pcb();
	init_thread(main_thread, "main", 31);
	
	ASSERT(!elem_find(&thread_all_list, &main_thread->all_list_tag));
	list_append(&thread_all_list, &main_thread->all_list_tag);
}