#ifndef __USERPROG_PROCESS_H
#define __USERPROG_PROCESS_H

#include "thread.h"

#define USER_VADDR_START 0x8048000

void start_process(void* filename_);
void page_dir_activate(struct task_struct* p_thread);
void process_activate(struct task_struct* p_thread);
void process_execute(void* filename, char* name);
uint32_t* create_page_dir(void);
void init(void);


#endif // __USERPROG_PROCESS_H
