#ifndef __KERNEL_MEMORY_H
#define __KERNEL_MEMORY_H

#include "stdint.h"
#include "bitmap.h"
#include "print.h"
#include "global.h"
#include "debug.h"
#include "string.h"

#define PG_P_1 1
#define PG_P_0 0
#define PG_RW_R 0
#define PG_RW_W (1 << 1)
#define PG_US_S 0
#define PG_US_U (1 << 2)

struct vaddr_pool
{
	struct bitmap vaddr_bitmap;
	uint32_t vaddr_start; 
};

enum pool_flags
{
	PF_KERNEL = 1,	
	PF_USER   = 2
};

extern struct paddr_pool k_p_pool, u_p_pool;

void mem_init(void);
uint32_t* pte_ptr(void* vaddr);
uint32_t* pde_ptr(void* vaddr);
void* malloc_page(enum pool_flags pf, uint32_t pg_cnt);
void* get_kernel_pages(uint32_t pg_cnt);

#endif // __KERNEL_MEMORY_H
