#include "sync.h"
#include "interrupt.h"
#include "stdint.h"
#include "thread.h"
#include "list.h"
#include "debug.h"
#include "print.h"

void sema_init(struct semaphore* psema, uint8_t value)
{ 
	psema->value = value;
	list_init(&psema->waiters);
}

void lock_init(struct lock* plock)
{
	plock->holder = NULL;
	plock->holder_repeat_cnt = 0;
	sema_init(&plock->lock_sema, 1); // binary semaphore
}

void sema_down(struct semaphore* psema)
{
	enum intr_status old_status = intr_disable();
	
	while(psema->value == 0) // lock is being holded by other thread
							 // need to use "while" instead of "if"
	{
		ASSERT(!elem_find(&psema->waiters, &get_cur_thread_pcb()->general_tag));
		if(elem_find(&psema->waiters, &get_cur_thread_pcb()->general_tag))
		{
			PANIC("sema_down: thread blocked has been in waiters_list\n");
		}
		list_append(&psema->waiters, &get_cur_thread_pcb()->general_tag);
		thread_block(TASK_BLOCKED);
	}
	
	psema->value--;
	intr_set_status(old_status);
}

void sema_up(struct semaphore* psema)
{
	enum intr_status old_status = intr_disable();
	
	ASSERT(psema->value == 0);
	
	if(!list_empty(&psema->waiters))
	{
		struct task_struct* thread_to_wake = elem2entry(struct task_struct, general_tag,\
														list_pop(&psema->waiters));
		thread_unblock(thread_to_wake); // wake thread and add it to thread_ready_list
										// the unblock thread may be blocked again for 
										// same reason if other thread compete with it 
	}
	psema->value ++;
	ASSERT(psema->value == 1);
	intr_set_status(old_status);
}

void lock_acquire(struct lock* plock)
{
	if(plock->holder != get_cur_thread_pcb()) // the thread acquire first time 
	{
		sema_down(&plock->lock_sema); // if other thread hold the lock 
		  							  // the thread will be blocked
		plock->holder = get_cur_thread_pcb();
		ASSERT(plock->holder_repeat_cnt == 0);
		plock->holder_repeat_cnt = 1;
	}else
	{
		plock->holder_repeat_cnt ++;
	}
}

void lock_release(struct lock* plock)
{
	ASSERT(plock->holder == get_cur_thread_pcb());		

	if((plock->holder_repeat_cnt--) == 1)
	{
		plock->holder = NULL;
		sema_up(&plock->lock_sema);
	}
}
