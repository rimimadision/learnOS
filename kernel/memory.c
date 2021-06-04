#include "memory.h"

#define PG_SIZE (1 << 12)

/* bitmap address : 0xc009a000 
   for PCB at 0xc009e00, we can allocate 0x4000 bytes for bitmap */
#define MEM_BITMAP_BASE 0xc009a000

/* already allocate 0xc0000000 ~  0xc00fffff for kernel, 
   set kernel heap start from 0xc0100000 */
#define K_HEAP_START 0xc0100000

struct paddr_pool
{
	struct bitmap paddr_bitmap;
	uint32_t paddr_start;
	uint32_t paddr_pool_size;	
};

struct paddr_pool k_p_pool, u_p_pool;
struct vaddr_pool k_v_pool;

static void mem_pool_init(uint32_t all_mem)
{
	put_str("mem_pool_init start\n");
	/* we already set 256 page_table and use low 1MB mem */
	uint32_t used_mem = PG_SIZE * 256 + 0x100000;
	uint32_t free_mem = all_mem - used_mem;
	uint16_t all_free_pages = free_mem / PG_SIZE;

	/* allocate pages for kernel and user equally */
	uint32_t kernel_free_pages = all_free_pages / 2;
	uint32_t user_free_pages = all_free_pages - kernel_free_pages;
	
	uint32_t k_btmp_len = kernel_free_pages / 8;
	uint32_t u_btmp_len = user_free_pages / 8;

	uint32_t k_p_start = used_mem;
	uint32_t u_p_start = used_mem + kernel_free_pages * PG_SIZE;
	
	/* initialize k_p_pool and u_p_pool */
	k_p_pool.paddr_bitmap.btmp_bytes_len = k_btmp_len;
	u_p_pool.paddr_bitmap.btmp_bytes_len = u_btmp_len;
	
	k_p_pool.paddr_bitmap.btmp_addr = (uint8_t*)MEM_BITMAP_BASE; 
	u_p_pool.paddr_bitmap.btmp_addr = (uint8_t*)(MEM_BITMAP_BASE + k_btmp_len);

	bitmap_init(&k_p_pool.paddr_bitmap);
	bitmap_init(&u_p_pool.paddr_bitmap);
	
	k_p_pool.paddr_start = k_p_start;
	u_p_pool.paddr_start = u_p_start;

	k_p_pool.paddr_pool_size = kernel_free_pages * PG_SIZE;
	u_p_pool.paddr_pool_size = user_free_pages   * PG_SIZE;

	put_str("physical memory pool info :\n    kernel_pool_bitmap_start: 0x");
	put_int((int)k_p_pool.paddr_bitmap.btmp_addr);
	put_str("\n    kernel_pool_phy_addr_start: 0x");
	put_int((int)k_p_start);
	put_str("\n    user_pool_bitmap_start: 0x");
	put_int((int)u_p_pool.paddr_bitmap.btmp_addr);
	put_str("\n    user_pool_phy_addr_start: 0x");
	put_int((int)u_p_start);
	put_str("\n");

	/* initialize k_v_pool */
	k_v_pool.vaddr_bitmap.btmp_bytes_len = k_btmp_len;
	k_v_pool.vaddr_bitmap.btmp_addr = (uint8_t*)(MEM_BITMAP_BASE + k_btmp_len + u_btmp_len);
	bitmap_init(&k_v_pool.vaddr_bitmap);
	k_v_pool.vaddr_start = K_HEAP_START;
	put_str("mem pool init done\n");
}

void mem_init()
{
	put_str("mem_init start\n");
	uint32_t mem_bytes_total = (*(uint32_t*) 0xb00);
	mem_pool_init(mem_bytes_total);
	put_str("mem_init done\n");
}
