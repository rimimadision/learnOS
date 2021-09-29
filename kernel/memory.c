#include "memory.h"
#include "sync.h"
#include "thread.h"
#include "debug.h"
#include "list.h"
#include "interrupt.h"

/*
-----------------------------------------------------------------|
1. |alloc_v_pages(allocate pages in virtual mem)  | -->			 |
2. |palloc(allocate physical pages)               | -->			 |
3. |page_table_add(map physical pages to virtual pages)|	     |
------------------------------------------------------------------
*/


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
	struct lock lock;
};

struct arena {
	struct mem_block_desc* desc;
	uint32_t cnt; // page_cnt for large arena, block_cnt for small arena
	bool large;
};

struct mem_block_desc k_block_desc[DESC_CNT];
struct paddr_pool k_p_pool, u_p_pool;
struct vaddr_pool k_v_pool;

/* local function */
static void mem_pool_init(uint32_t all_mem);
static void* alloc_v_pages(enum pool_flags pf, uint32_t pg_cnt);
static void* palloc(struct paddr_pool* ptr_p_pool);
static void page_table_add(void* _vaddr, void* _page_phyaddr);
static void page_table_pte_remove(uint32_t vaddr);
static struct mem_block* arena2block(struct arena* a, uint32_t idx);
static struct arena* block2arena(struct mem_block* b);
static void vaddr_remove(enum pool_flags pf, void* _vaddr, uint32_t pg_cnt);

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

	/* initialize lock */
	lock_init(&k_p_pool.lock);
	lock_init(&u_p_pool.lock);

	put_str("mem pool init done\n");
}

void block_desc_init(struct mem_block_desc* desc_array) {
	uint16_t desc_idx, block_size = 16;
	
	for(desc_idx = 0; desc_idx < DESC_CNT; desc_idx++) {
		desc_array[desc_idx].block_size = block_size;
		desc_array[desc_idx].blocks_per_arena = (PG_SIZE - sizeof(struct arena)) / block_size;
		list_init(&desc_array[desc_idx].free_list);
		block_size *= 2;
	}
}
void mem_init()
{
	put_str("mem_init start\n");
	uint32_t mem_bytes_total = (*(uint32_t*) 0xb00);
	mem_pool_init(mem_bytes_total);
	block_desc_init(k_block_desc);
	put_str("mem_init done\n");
}

static struct mem_block* arena2block(struct arena* a, uint32_t idx) {
	return (struct mem_block*)((uint32_t)a + sizeof(struct arena) + idx *(a->desc->block_size));	
}

static struct arena* block2arena(struct mem_block* b) {
	return (struct arena*)((uint32_t)b & 0xfffff000);
}

/* return the start address if sucess, or else return NULL */
static void* alloc_v_pages(enum pool_flags pf, uint32_t pg_cnt)
{
	void* alloc_vaddr_start = NULL;
	int bit_idx_start = -1;
	uint32_t cnt = 0;

	if(pf == PF_KERNEL)
	{
		bit_idx_start = bitmap_scan(&k_v_pool.vaddr_bitmap, pg_cnt);
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
		struct task_struct* cur = get_cur_thread_pcb();
		bit_idx_start = bitmap_scan(&cur->userprog_vaddr.vaddr_bitmap, pg_cnt);
		if (bit_idx_start == -1) {
			return NULL;
		}

		while (cnt < pg_cnt) {
			bitmap_set(&cur->userprog_vaddr.vaddr_bitmap, bit_idx_start + cnt++, 1);
		}
		alloc_vaddr_start = (void*)(cur->userprog_vaddr.vaddr_start + bit_idx_start * PG_SIZE);
		ASSERT((uint32_t)alloc_vaddr_start < (0xc0000000 - PG_SIZE));
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

void pfree(uint32_t pg_phy_addr) {
	struct paddr_pool* mem_pool;
	uint32_t bit_idx = 0;

	if(pg_phy_addr >= u_p_pool.paddr_start) {
		mem_pool = &u_p_pool;	
		bit_idx = (pg_phy_addr - u_p_pool.paddr_start) / PG_SIZE;
	} else {
		mem_pool = &k_p_pool;
		bit_idx = (pg_phy_addr - k_p_pool.paddr_start) / PG_SIZE;
	}
	bitmap_set(&mem_pool->paddr_bitmap, bit_idx, 0);
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

static void page_table_pte_remove(uint32_t vaddr) {
	uint32_t* pte = pte_ptr((void*)vaddr);	
	ASSERT(*pte & 0x1);
	*pte &= ~PG_P_1;
	asm volatile("invlpg %0"::"m"(vaddr):"memory"); // fresh TLB
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

/* free pg_cnt pages in v_pool start with _vaddr */
static void vaddr_remove(enum pool_flags pf, void* _vaddr, uint32_t pg_cnt) {
	uint32_t bit_idx_start = 0, vaddr = (uint32_t)_vaddr, cnt = 0;
	
	if(pf == PF_KERNEL) {
		bit_idx_start = (vaddr - k_v_pool.vaddr_start) / PG_SIZE;
		while(cnt < pg_cnt) {
			bitmap_set(&k_v_pool.vaddr_bitmap, bit_idx_start + cnt++, 0);
		}
	} else {
		struct task_struct* cur_thread = get_cur_thread_pcb();
		bit_idx_start = (vaddr - cur_thread->userprog_vaddr.vaddr_start) / PG_SIZE;
		while(cnt < pg_cnt) {
			bitmap_set(&cur_thread->userprog_vaddr.vaddr_bitmap, bit_idx_start + cnt++, 0);
		}
	}
}

void mfree_page(enum pool_flags pf, void* _vaddr, uint32_t pg_cnt) {
	uint32_t pg_phy_addr;
	uint32_t vaddr = (uint32_t)_vaddr;
	uint32_t page_cnt = 0;

	ASSERT(pg_cnt >= 1 && vaddr % PG_SIZE == 0);
		
	pg_phy_addr = addr_v2p(vaddr);

	ASSERT((pg_phy_addr % PG_SIZE) == 0 && pg_phy_addr >= 0x102000); // 0x102000 = 1MB + 1KB(PD) + 1KB(PT)

	if(pg_phy_addr >= u_p_pool.paddr_start) {
		vaddr -= PG_SIZE;

		while(page_cnt < pg_cnt) {
			vaddr += PG_SIZE;
			pg_phy_addr = addr_v2p(vaddr);
			ASSERT((pg_phy_addr % PG_SIZE) == 0 && pg_phy_addr >= u_p_pool.paddr_start);
			pfree(pg_phy_addr);
			page_table_pte_remove(vaddr);
			page_cnt++;
		}

		vaddr_remove(pf, _vaddr, pg_cnt);
	} else {
		vaddr -= PG_SIZE;
		while(page_cnt < pg_cnt) {
			vaddr += PG_SIZE;
			pg_phy_addr = addr_v2p(vaddr);
			ASSERT((pg_phy_addr % PG_SIZE) == 0 && pg_phy_addr >= k_p_pool.paddr_start && pg_phy_addr < u_p_pool.paddr_start);
			pfree(pg_phy_addr);
			page_table_pte_remove(vaddr);
			page_cnt++;
		}

		vaddr_remove(pf, _vaddr, pg_cnt);
	}
}


void* get_kernel_pages(uint32_t pg_cnt)
{
	lock_acquire(&k_p_pool.lock);
	
	void* vaddr_start = malloc_page(PF_KERNEL, pg_cnt);
	if(vaddr_start != NULL)
	{
		memset(vaddr_start, 0, pg_cnt * PG_SIZE);
	}

	lock_release(&k_p_pool.lock);
	return vaddr_start;
}

void* get_user_pages(uint32_t pg_cnt) {
	lock_acquire(&u_p_pool.lock);

	void* vaddr_start = malloc_page(PF_USER, pg_cnt);
	if(vaddr_start != NULL) {
		memset(vaddr_start, 0, pg_cnt * PG_SIZE);
	}
	
	lock_release(&u_p_pool.lock);
	return vaddr_start;
}

/* get specific virtual address alloced */
void* get_a_page(enum pool_flags pf, uint32_t vaddr) {
	struct paddr_pool* mem_pool = (pf == PF_KERNEL) ? &k_p_pool : &u_p_pool;
	lock_acquire(&mem_pool->lock);
	
	struct task_struct* cur = get_cur_thread_pcb();
	int32_t bit_idx = -1;
	
	if (cur->pgdir != NULL && pf == PF_USER) { // user progress ask for 1 page
		bit_idx = (vaddr - cur->userprog_vaddr.vaddr_start) / PG_SIZE;
		ASSERT(bit_idx > 0);
		bitmap_set(&cur->userprog_vaddr.vaddr_bitmap, bit_idx, 1);
	} else if (cur->pgdir == NULL && pf == PF_KERNEL) { // kernel thread ask for 1 page
		bit_idx = (vaddr - k_v_pool.vaddr_start) / PG_SIZE;
		ASSERT(bit_idx > 0);
		bitmap_set(&k_v_pool.vaddr_bitmap, bit_idx, 1);
	} else {
		PANIC("get_a_page:not allow kernel alloc userspace or user alloc kernelspace");
	}

	void* page_phyaddr = palloc(mem_pool);
	if(page_phyaddr == NULL) {
		// Need to do rollback here, but i'm lazy ^_^
		return NULL;
	}
	page_table_add((void*)vaddr, page_phyaddr);
	lock_release(&mem_pool->lock);

	return (void*)vaddr;
}

uint32_t addr_v2p(uint32_t vaddr) {
	uint32_t* pte = pte_ptr((void*)vaddr);
	return ((*pte & 0xfffff000) + (vaddr & 0xfff));
}

void* sys_malloc(uint32_t size) {
	enum pool_flags PF;
	struct paddr_pool* mem_pool;
	uint32_t pool_size;
	struct mem_block_desc* desc;
	struct task_struct* cur_thread = get_cur_thread_pcb(); 

	if(cur_thread->pgdir == NULL) {
		PF = PF_KERNEL;
		mem_pool = &k_p_pool;
		pool_size = k_p_pool.paddr_pool_size;
		desc = k_block_desc;
	} else {
		PF = PF_USER;
		pool_size = u_p_pool.paddr_pool_size;
		mem_pool = &u_p_pool;
		desc = cur_thread->u_block_desc;
	}
	
	if(!(size > 0 && size < pool_size)) {
		return NULL;
	}
	
	struct arena* a;
	struct mem_block* b;
	lock_acquire(&mem_pool->lock);
	
	if(size > 1024) { // alloc pages
		uint32_t page_cnt = DIV_ROUND_UP(size + sizeof(struct arena), PG_SIZE);
		a = (struct arena*)malloc_page(PF, page_cnt);
		if(a != NULL) {
			
			memset(a, 0, page_cnt * PG_SIZE);		
	
			//initialize arena
			a->desc = NULL;
			a->cnt = page_cnt;
			a->large = true;
			a += 1;
		}
		lock_release(&mem_pool->lock);
		return (void*)a;
	} else { // alloc mem_space in one page
		uint8_t desc_idx;
		
		for(desc_idx = 0; desc_idx < DESC_CNT; desc_idx++) {
			if(size <= desc[desc_idx].block_size){break;}
		}
		
		if(list_empty(&desc[desc_idx].free_list)) {
			a = malloc_page(PF, 1);
			if(a == NULL) {
				lock_release(&mem_pool->lock);
				return NULL;
			} 
			
			memset(a, 0, PG_SIZE);
			
			a->desc = &desc[desc_idx];
			a->large = false;
			a->cnt = desc[desc_idx].blocks_per_arena;

			uint32_t block_idx;
				
			enum intr_status old_status = intr_disable();
			for(block_idx = 0; block_idx < desc[desc_idx].blocks_per_arena; block_idx++) {
				b = arena2block(a, block_idx);
				ASSERT(!elem_find(&a->desc->free_list, &b->free_elem));
				list_append(&a->desc->free_list, &b->free_elem);
			}
			intr_set_status(old_status);
		}

		b = elem2entry(struct mem_block, free_elem, list_pop(&desc[desc_idx].free_list));
		memset(b, 0, desc[desc_idx].block_size);
		a = block2arena(b);
		a->cnt--;
		lock_release(&mem_pool->lock);
		
		return (void*)b;	
	}
}

void sys_free(void* ptr) {
	ASSERT(ptr != NULL);
	if(ptr == NULL) {return;}
	enum pool_flags pf;
	struct paddr_pool* mem_pool;
	
	if(get_cur_thread_pcb()->pgdir == NULL) { // kernel thread
		pf = PF_KERNEL;
		mem_pool = &k_p_pool;
	} else {
		pf = PF_USER;
		mem_pool = &u_p_pool;
	}

	lock_acquire(&mem_pool->lock);
	struct mem_block* b = ptr;
	struct arena* a = block2arena(b);
	
	if(a->desc == NULL && a->large == true) {
		mfree_page(pf, a, a->cnt);
	} else {
		list_append(&a->desc->free_list, &b->free_elem);

		if(++a->cnt == a->desc->blocks_per_arena) {
			uint32_t block_idx;
			for(block_idx = 0; block_idx < a->desc->blocks_per_arena; block_idx++) {
				struct mem_block* b = arena2block(a, block_idx);
				ASSERT(elem_find(&a->desc->free_list, &b->free_elem));
				list_remove(&b->free_elem);
			}
			mfree_page(pf, a, 1);
		}
	}
	lock_release(&mem_pool->lock);
}

void* get_a_page_without_opvaddrbitmap(enum pool_flags pf, uint32_t vaddr) {
	struct paddr_pool* mem_pool = pf & PF_KERNEL ? &k_p_pool : &u_p_pool;
	lock_acquire(&mem_pool->lock);
	void* page_phyaddr = palloc(mem_pool);
	if (page_phyaddr == NULL) {
		lock_release(&mem_pool->lock);
		return NULL;
	}
	page_table_add((void*)vaddr, page_phyaddr);
	lock_release(&mem_pool->lock);
	
	return (void*)vaddr;
}

void free_a_phy_page(uint32_t pg_phy_addr) {
	pfree(pg_phy_addr);
}
