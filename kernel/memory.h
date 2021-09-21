#ifndef __KERNEL_MEMORY_H
#define __KERNEL_MEMORY_H

#include "stdint.h"
#include "bitmap.h"
#include "print.h"
#include "global.h"
#include "debug.h"
#include "string.h"
#include "list.h"

#define PG_SIZE (1 << 12)

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

struct mem_block {
	struct list_elem free_elem;
};

struct mem_block_desc {
	uint32_t block_size; 
	uint32_t blocks_per_arena;
	struct list free_list;
};

#define DESC_CNT 7 // 16, 32, 64, 128, 256, 512, 1024 (MB)

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
void pfree(uint32_t pg_phy_addr);
void* get_kernel_pages(uint32_t pg_cnt);
void* get_user_pages(uint32_t pg_cnt);
void* get_a_page(enum pool_flags pf, uint32_t vaddr);
uint32_t addr_v2p(uint32_t vaddr);
void block_desc_init(struct mem_block_desc* desc_array);
void mfree_page(enum pool_flags pf, void* _vaddr, uint32_t pg_cnt);
void sys_free(void* ptr);
void* sys_malloc(uint32_t size);
void* get_a_page_without_opvaddrbitmap(enum pool_flags pf, uint32_t vaddr);
#endif // __KERNEL_MEMORY_H
