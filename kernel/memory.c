#include "memory.h"

/*
------------------------------------------------------------------
process of malloc_page                                           |
																 |
1. |alloc_v_pages(allocate pages in virtual mem)  | -->			 |
2. |palloc(allocate physical pages)               | -->			 |
3. |page_table_add(map physical pages to virtual pages)|	     |
------------------------------------------------------------------
*/

#define PG_SIZE (1 << 12)

/* bitmap address : 0xc009a000 
   for PCB at 0xc009e00, we can allocate 0x4000 bytes for bitmap */
#define MEM_BITMAP_BASE 0xc009a000

/* already allocate 0xc0000000 ~  0xc00fffff for kernel, 
   set kernel heap start from 0xc0100000 */
#define K_HEAP_START 0xc0100000

#define PDE_IDX(addr) (((uint32_t)addr & 0xffc00000) >> 22)
#define PTE_IDX(addr) (((uint32_t)addr & 0x003ff000) >> 12)

struct paddr_pool
{
	struct bitmap paddr_bitmap;
	uint32_t paddr_start;
	uint32_t paddr_pool_size;	
};

struct paddr_pool k_p_pool, u_p_pool;
struct vaddr_pool k_v_pool;

/* local function */
static void mem_pool_init(uint32_t all_mem);
static void* alloc_v_pages(enum pool_flags pf, uint32_t pg_cnt);
static void* palloc(struct paddr_pool* ptr_p_pool);
static void page_table_add(void* _vaddr, void* _page_phyaddr);

/* global funtion declarations are in kernel/memory.h */

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

/* return the start address if sucess, or else return NULL */
static void* alloc_v_pages(enum pool_flags pf, uint32_t pg_cnt)
{
	void* alloc_vaddr_start = NULL;
	if(pf == PF_KERNEL)
	{
		int bit_idx_start = bitmap_scan(&k_v_pool.vaddr_bitmap, pg_cnt);
		ASSERT(bit_idx_start != -1);
		if(bit_idx_start == -1)
		{
			return NULL;
		}
		alloc_vaddr_start = (void*)(k_v_pool.vaddr_start + bit_idx_start * PG_SIZE);
		uint32_t bit_idx = bit_idx_start;
		while(pg_cnt--)
		{
			bitmap_set(&k_v_pool.vaddr_bitmap, bit_idx++, 1);
		}
	}else // pf == PF_USER
	{
		// DO USER VIRTUAL MEMORY ALLOC
	}

	return alloc_vaddr_start;
}

/* return page table entry(4 bytes) of vaddr */
inline uint32_t* pte_ptr(void* vaddr)
{
	return (uint32_t*)(0xffc00000 + (((uint32_t)vaddr & 0xffc00000) >> 10)\
			+ PTE_IDX((uint32_t)vaddr) * 4);
}

/* return page directory entry(4 bytes) of vaddr */
inline uint32_t* pde_ptr(void* vaddr)
{
	return (uint32_t*)(0xfffff000 + PDE_IDX((uint32_t)vaddr) * 4);
}

/* alloc physical page, return physical address if sucess, or else return NULL
   can only alloc one page once for physical address can be not continuous          
*/
static void* palloc(struct paddr_pool* ptr_p_pool)
{
	int bit_idx_start = bitmap_scan(&ptr_p_pool->paddr_bitmap, 1);
	if(bit_idx_start == -1)
	{
		return NULL;
	}
	bitmap_set(&ptr_p_pool->paddr_bitmap, bit_idx_start, 1);

	return (void*)(ptr_p_pool->paddr_start + bit_idx_start * PG_SIZE);
}

/* mapping _vaddr to _page_phyaddr */
static void page_table_add(void* _vaddr, void* _page_phyaddr)
{
	uint32_t* pde = pde_ptr(_vaddr);
	uint32_t* pte = pte_ptr(_vaddr);
  	
	/* judge if pde exists, if not exist, create pde*/
	if(!(*pde & 0x1))
	{
		uint32_t new_pt_phyaddr = (uint32_t)palloc(&k_p_pool);
		*pde = (new_pt_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
		memset((void*)((uint32_t)pte & 0xfffff000), 0, PG_SIZE);
	}
	ASSERT(!(*pte & 0x1));
	if(*pte & 0x1)
	{
		PANIC("pte repeat");
	}
	*pte = ((uint32_t)_page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
}

void* malloc_page(enum pool_flags pf, uint32_t pg_cnt)
{
	ASSERT(pg_cnt > 0 && pg_cnt < 3840); // 3840 = 15MB / 4kb
										 //limit the space alloced smaller than 15MB 
	/* alloc virtual memory */
	void* alloc_vaddr_start = alloc_v_pages(pf, pg_cnt);
	if(alloc_vaddr_start == NULL)
	{
		return NULL;
	}

	uint32_t vaddr = (uint32_t)alloc_vaddr_start;
	uint32_t cnt = pg_cnt;
	struct paddr_pool* p_pool = (pf == PF_KERNEL) ? &k_p_pool : &u_p_pool;
	
	while(cnt--)
	{
		void* page_phyaddr = palloc(p_pool);
		
		if(page_phyaddr == NULL)
		{
			// DO MEMORY_COLLECT
			return NULL;
		}
		page_table_add((void*)vaddr, page_phyaddr);
		
		vaddr += PG_SIZE;
	}

	return alloc_vaddr_start;
}

void* get_kernel_pages(uint32_t pg_cnt)
{
	void* vaddr_start = malloc_page(PF_KERNEL, pg_cnt);
	if(vaddr_start != NULL)
	{
		memset(vaddr_start, 0, pg_cnt * PG_SIZE);
	}
	return vaddr_start;
}
