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
}

void mem_init()
{
	put_str("mem_init start\n");
	uint32_t mem_bytes_total = (*(uint32_t*) 0xb00);
	mem_pool_init(mem_bytes_total);
	put_str("mem_init done\n");
}
