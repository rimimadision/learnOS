#ifndef __THREAD_THREAD_H
#define __THREAD_THREAD_H

#include "stdint.h"
#include "list.h"
#include "bitmap.h"
#include "memory.h"

#define offset(struct_type, member) (int)(&((struct_type*)0)->member)
#define elem2entry(struct_type, member, elem_ptr)\
		(struct_type*)((int)elem_ptr - offset(struct_type, member))

typedef int16_t pid_t;
typedef void thread_func(void*); // universal function type

extern struct list thread_ready_list;
extern struct list thread_all_list;

enum task_status
{
	TASK_RUNNING,
	TASK_READY,
	TASK_BLOCKED,
	TASK_WAITING,
	TASK_HANGING,
	TASK_DIED
};

/* stack for saving context when interrupt occur */
struct intr_stack
{ 
	uint32_t vec_no;

	/* pushad */
	uint32_t edi;
	uint32_t esi;
	uint32_t ebp;
	uint32_t esp_dummy; // esp is pushed when running pushad,
						// popad will ignore it
	uint32_t ebx;
	uint32_t edx;
	uint32_t ecx;
	uint32_t eax;

	/* sreg */
	uint32_t gs;
	uint32_t fs;
	uint32_t es;
	uint32_t ds;

	/* CPU push automatically */
	uint32_t err_code; // err_code = 0 if CPU don't push err_code
	void (*eip)(void); // the return address
	uint32_t cs;
	uint32_t eflags;
	void* esp; // push esp and ss when privilege level gets higher
	uint32_t ss;
};

/* stack for thread information */
struct thread_stack
{
	/* When use C program call ASM function,
	 * need to protect ebp, ebx, edi, esi and esp,
	 * for esp won't change after 'ret' from ASM function,
	 * only need to protect other four registers.
	 */
	uint32_t ebp;	
	uint32_t ebx;
	uint32_t edi;
	uint32_t esi;

	/* When thread runs at its first time,
	 * "eip"(not current eip reg) points to kernel_thread to 
	 * start the thread_func; in the other time, "eip" points
	 * to switch_to function's return address(or called next_thread
	 * function address)
	 */
	void (*eip)(thread_func* func, void* func_arg);

	/* var below will be used only when thread is scheduled at its first time */
	void (*unused_retaddr); // placeholder used in kernel_thread, will not be used
	thread_func* function; // main function for this thread
	void* func_arg; // arguments for "function" above
};

/* Process Control Block, size = 1 page */
struct task_struct
{
	uint32_t* self_kstack; // every kernel_thread will have its own kernel_stack 
						   // with privilege_level = 0
	pid_t pid;
	enum task_status status;
	char name[16];
	uint8_t priority;
	
	uint8_t ticks;
	uint32_t elapsed_ticks;
	
	struct list_elem general_tag; // used in thread_ready_list 	
	struct list_elem all_list_tag; // used in thread_all_list
	
	uint32_t* pgdir; // vaddr of task's page_table(if task is a process, it wiil have its own 
					 // virtual space, if task is a thread, then set pgdir = NULL)
	struct vaddr_pool userprog_vaddr;
	struct mem_block_desc u_block_desc[DESC_CNT]; 
	uint32_t stack_magic; // 0x19700505
};

struct task_struct* thread_start(char* name, int prio, thread_func* function, void* func_arg);
struct task_struct* get_cur_thread_pcb(void);
void schedule(void);
void thread_init(void);
void thread_block(enum task_status stat);
void thread_unblock(struct task_struct* pthread);
void init_thread(struct task_struct* pthread, char* name, int prio);
void thread_create(struct task_struct* pthread, thread_func* function, void* func_arg);
void thread_yield(void);

#endif // __THREAD_THREAD_H
