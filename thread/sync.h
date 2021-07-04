#ifndef __THREAD_SYNC_H
#define __THREAD_SYNC_H

#include "list.h"
#include "stdint.h"
#include "thread.h"

struct semaphore
{
	uint8_t value;
	struct list waiters;
};

struct lock 
{
	struct task_struct* holder; // PCB of lock_holder
	struct semaphore lock_sema; 
	uint32_t holder_repeat_cnt;
};

void lock_init(struct lock* plock);
void lock_acquire(struct lock* plock);
void lock_release(struct lock* plock);

#endif // __THREAD_SYNC_H
