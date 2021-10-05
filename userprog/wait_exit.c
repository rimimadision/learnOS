#include "wait_exit.h"
#include "thread.h"
#include "stdint.h"
#include "global.h"
#include "memory.h"

static void release_prog_resource(struct task_struct* release_thread) {
	uint32_t* pgdir_vaddr = release_thread->pgdir;
	uint16_t user_pde_nr = 768, pde_idx = 0;
	uint32_t pde = 0;
	uint32_t* v_pde_ptr = NULL;

	uint16_t user_pte_nr = 1024, pte_idx = 0;
	uint32_t pte = 0;
	uint32_t* v_pte_ptr = NULL;

	uint32_t* first_pte_vaddr_in_pde = NULL;
	uint32_t pg_phy_addr = 0;

	while (pde_idx < user_pde_nr) {
		v_pde_ptr = pgdir_vaddr + pde_idx;
		pde = *v_pde_ptr;
		if (pde & 0x00000001) {
			first_pte_vaddr_in_pde = pte_ptr(pde_idx * 0x400000);
			pte_idx = 0;
			while (pte_idx < user_pte_nr) {
				v_pte_ptr = first_pte_vaddr_in_pde + pte_idx;
				pte = *v_pte_ptr;
				if (pte & 0x00000001) {
					pg_phy_addr = pte & 0xfffff000;
					free_a_phy_page(pg_phy_addr);
				}
				pte_idx++;
			}
		
			pg_phy_addr = pde & 0xfffff000;
			free_a_phy_page(pg_phy_addr);
		}	
		pde_idx++;
	}
	uint32_t bitmap_pg_cnt = (release_thread->userprog_vaddr.vaddr_bitmap.btmp_bytes_lena) / PG_SIZE;
	uint8_t* user_vaddr_pool_bitmap = release_thread->user_vaddr.vaddr_bitmap.btmp_addr;
	mfree_page(PF_KERNEL, user_vaddr_pool_bitmap, bitmap_pg_cnt);	

	uint8_t fd_idx = 3;
 	while (fd_idx < MAX_FILES_OPEN_PER_PROC) {
		if (release_thread->fd_table[fd_idx] != -1) {
			sys_close(fd_idx);
		}
		fd_idx++;
	}
} 

