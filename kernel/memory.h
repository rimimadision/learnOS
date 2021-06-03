#ifndef __KERNEL_MEMORY_H
#define __KERNEL_MEMORY_H

#include "stdint.h"
#include "bitmap.h"
#include "print.h"

struct vaddr_pool
{
	struct bitmap vaddr_bitmap;
	uint32_t vaddr_start; 
};

extern struct paddr_pool k_p_pool, u_p_pool;

void mem_init(void);

#endif // __KERNEL_MEMORY_H
