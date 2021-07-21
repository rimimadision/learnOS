#ifndef __DEVICE_IOQUEUE_H
#define __DEVICE_IOQUEUE_H
#include "stdint.h"
#include "thread.h"
#include "sync.h"
#include "global.h"

#define bufsize 64

struct ioqueue {
	struct lock lock;
	struct task_struct* producer;
	struct task_struct* consumer;
	char buf[bufsize];
	int32_t head;
	int32_t tail;
};

void ioqueue_init(struct ioqueue* ioq);
char ioq_getchar(struct ioqueue* ioq);
void ioq_putchar(struct ioqueue* ioq, char byte);
bool ioq_full(struct ioqueue* ioq);
bool ioq_empty(struct ioqueue* ioq);

#endif // __DEVICE_IOQUEUE_H

