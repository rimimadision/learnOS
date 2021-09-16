#include "thread.h"
#include "stdint.h"
#include "string.h"
#include "global.h"
#include "memory.h"
#include "list.h"
#include "interrupt.h"
#include "process.h"
#include "sync.h"

struct task_struct* main_thread;
struct task_struct* idle_thread;
struct list thread_ready_list;
struct list thread_all_list;
struct lock pid_lock;

extern void switch_to(struct task_struct* cur, struct task_struct* next);

static void kernel_thread(thread_func* function, void* func_arg);
static void make_main_thread(void);
static void idle(void* arg UNUSED);
static pid_t allocate_pid(void);

void thread_create(struct task_struct* pthread, thread_func* function, void* func_arg);
void init_thread(struct task_struct* pthread, char* name, int prio);
void thread_block(enum task_status stat);
void thread_unblock(struct task_struct* pthread);

static void idle(void* arg UNUSED) {
	while(1) {
		thread_block(TASK_BLOCKED);
		asm volatile ("sti; hlt" : : : "memory");
	}
}

/* get current thread PCB */
struct task_struct* get_cur_thread_pcb(void)
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

static pid_t allocate_pid(void) {
	static pid_t next_pid = 0;
	lock_acquire(&pid_lock);
	next_pid++;
	lock_release(&pid_lock);
	return next_pid;
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
	pthread->pid = allocate_pid();
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
	
	/* init fd_table */
	pthread->fd_table[0] = 0;
	pthread->fd_table[1] = 1;
	pthread->fd_table[2] = 2;
    // fd = -1 means empty
	uint8_t fd_idx = 3;
	while (fd_idx < MAX_FILES_OPEN_PER_PROC) {
		pthread->fd_table[fd_idx] = -1;
		fd_idx++;
	}
	pthread->cwd_inode_nr = 0;
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

void schedule(void)
{
	ASSERT(intr_get_status() == INTR_OFF);
	
	struct task_struct* cur = get_cur_thread_pcb();
	if(cur->status == TASK_RUNNING)
	{
		ASSERT(!elem_find(&thread_ready_list, &cur->general_tag));
		list_append(&thread_ready_list, &cur->general_tag);
		cur->ticks = cur->priority;
		cur->status = TASK_READY;
	}else
	{
		//if status is not TASK_RUNNING, means that the thread may be blocked for 
		// some reason, then we won't append it into thread_ready_list
	}
	
	if(list_empty(&thread_ready_list)) {
		thread_unblock(idle_thread);
	}
	struct list_elem* next_tag = list_pop(&thread_ready_list);
	struct task_struct* next = elem2entry(struct task_struct, general_tag, next_tag);
	next->status = TASK_RUNNING;
	
	process_activate(next);
	
	switch_to(cur, next);
}

void thread_init(void)
{
	put_str("thread_init start\n");
	list_init(&thread_ready_list);
	list_init(&thread_all_list);
	lock_init(&pid_lock);
	make_main_thread();
	idle_thread = thread_start("idle", 10, idle, NULL);
	put_str("thread_init done\n");
}

/* block cur_thread and set status 'stat' */
void thread_block(enum task_status stat)
{
	ASSERT((stat == TASK_BLOCKED) || (stat == TASK_WAITING) || (stat == TASK_HANGING));
	enum intr_status old_status = intr_disable();
	struct task_struct* cur_thread = get_cur_thread_pcb();
	cur_thread->status = stat;
	schedule();
	intr_set_status(old_status);
}

/* unblock pthread */
void thread_unblock(struct task_struct* pthread)
{
	enum intr_status old_status = intr_disable();
	ASSERT((pthread->status == TASK_BLOCKED) || (pthread->status == TASK_WAITING) \
	       || (pthread->status == TASK_HANGING));
	if(pthread->status != TASK_READY)
	{
		ASSERT(!elem_find(&thread_ready_list, &pthread->general_tag));
		if(elem_find(&thread_ready_list, &pthread->general_tag))
		{
			PANIC("thread_unblock:blocked thread in ready_list\n");
		}	
		list_push(&thread_ready_list, &pthread->general_tag);
		pthread->status = TASK_READY;
	}

	intr_set_status(old_status);
}

void thread_yield(void) {
	struct task_struct* cur_thread = get_cur_thread_pcb();
	enum intr_status old_status = intr_disable();
	ASSERT(!elem_find(&thread_ready_list, &cur_thread->general_tag));
	list_append(&thread_ready_list, &cur_thread->general_tag);
	cur_thread->status = TASK_READY;
	schedule();
	intr_set_status(old_status);	
}
