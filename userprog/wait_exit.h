#ifndef __USERPROG_WAIT_EXIT_H
#define __USERPROG_WAIT_EXIT_H

#include "stdint.h"

int16_t sys_wait(int32_t* status);
void sys_exit(int32_t status);

#endif // __USERPROG_WAIT_EXIT_H
