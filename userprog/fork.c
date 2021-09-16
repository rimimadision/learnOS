#include "fork.h"
#include "thread.h"
#include "memory.h"
#include "bitmap.h"
#include "fs.h"
#include "process.h"
#include "file.h"

extern void intr_exit(void);

static int32_t copy_pcb_vaddrbitmap_stack0(struct task_struct* child_thread, struct task_struct* parent_thread);

static int32_t copy_pcb_vaddrbitmap_stack0(struct task_struct* child_thread, struct task_struct* parent_thread) {
	memcpy(child_thread, parent_thread, PG_SIZE);
	child_thread->pid = fork_pid();
	child_thread->elapsed_ticks = 0;
	child_thread->status = TASK_READY;
	child_thread->ticks = child_thread->priority;
	child_thread->parent_pid = parent_thread->pid;
	child_thread->general_tag.prev = \
	child_thread->general_tag.next = NULL;
	child_thread->all_list_tag.prev = \
	child_thread->all_list_tag.next = NULL;
	block_desc_init(child_thread->u_block_desc);
	
	uint32_t bitmap_pg_cnt = DIV_ROUND_UP((0xc0000000 - USER_VADDR_START) / PG_SIZE / 8, PG_SIZE);
	void* vaddr_btmp = get_kernel_pages(bitmap_pg_cnt);
	memcpy(vaddr_btmp, child_thread->userprog_vaddr.vaddr_bitmap.btmp_addr, bitmap_pg_cnt * PG_SIZE);
	child_thread->userprog_vaddr.vaddr_bitmap.btmp_addr = vaddr_btmp;
	ASSERT(strlen(child_thread->name) < 11);
	strcat(child_thread->name, "fork");
	return 0;	
}

static void copy_body_stack3(struct task_struct* child_thread, struct task_struct* parent_thread, void* buf_page) {
	uint8_t* vaddr_btmp = parent_thread->userprog_vaddr.vaddr_bitmap.btmp_addr;
	uint32_t btmp_bytes_len = parent_thread->userprog_vaddr.vaddr_bitmap.btmp_bytes_len;
	uint32_t vaddr_start = parent_thread->userprog_vaddr.vaddr_start;
	uint32_t idx_byte = 0;
	uint32_t idx_bit = 0;
	uint32_t prog_vaddr = 0;

	while (idx_byte < btmp_bytes_len) {
		if (vaddr_btmp[idx_byte]) {
			idx_bit = 0;
			while (idx_bit < 8) {
				if((BITMAP_MASK << idx_bit) & vaddr_btmp[idx_byte]) {
					prog_vaddr = (idx_byte * 8 + idx_bit) * PG_SIZE + vaddr_start;
					// we can still access to data in parent by copying them into 
					// buf_page, when we change into child's page directory
					memcpy(buf_page, (void*)prog_vaddr, PG_SIZE);
					page_dir_activate(child_thread);
					get_a_page_without_opvaddrbitmap(PF_USER, prog_vaddr);
					memcpy((void*)prog_vaddr, buf_page, PG_SIZE);
					page_dir_activate(parent_thread);					
				}
				idx_bit++;
			}
		}
		idx_byte++;
	}
}

static int32_t build_child_stack(struct task_struct* child_thread) {
	struct intr_stack* intr_0_stack = (struct intr_stack*)((uint32_t)child_thread + PG_SIZE - sizeof(struct intr_stack));
	intr_0_stack->eax = 0;
	uint32_t* ret_addr_in_thread_stack = (uint32_t*)intr_0_stack - 1;
	uint32_t* ebp_ptr_in_thread_stack = (uint32_t*)intr_0_stack - 5;

	*ret_addr_in_thread_stack = (uint32_t)intr_exit;
	child_thread->self_kstack = ebp_ptr_in_thread_stack;
	return 0;
}

static void update_inode_open_cnts(struct task_struct* thread) {
	int32_t local_fd = 3, global_fd = 0;
	while (local_fd < MAX_FILES_OPEN_PER_PROC) {
		global_fd = thread->fd_table[local_fd];
		ASSERT(global_fd < MAX_FILE_OPEN);
		if (global_fd != -1) {
			file_table[global_fd].fd_inode->i_open_cnts++; 
		}
		local_fd++;
	}	 
}

static int32_t copy_process(struct task_struct* child_thread, struct task_struct* parent_thread) {
	void* buf_page = get_kernel_pages(1);
	if (buf_page == NULL) {
		return -1;
	}

	if (copy_pcb_vaddrbitmap_stack0(child_thread, parent_thread) == -1) {
		mfree_page(PF_KERNEL, buf_page, 1);
		return -1;
	}

	child_thread->pgdir = create_page_dir();
	if (child_thread->pgdir == NULL) {
		mfree_page(PF_KERNEL, buf_page, 1);
		mfree_page(PF_KERNEL, child_thread->pgdir, 1);
		return -1;
	}

	copy_body_stack3(child_thread, parent_thread, buf_page);
	build_child_stack(child_thread);
	update_inode_open_cnts(child_thread);
	mfree_page(PF_KERNEL, buf_page, 1);

	return 0;
}

pid_t sys_fork(void) {
	struct task_struct* parent_thread = get_cur_thread_pcb();
	struct task_struct* child_thread = get_kernel_pages(1);

	if (child_thread == NULL) {
		mfree_page(PF_KERNEL, child_thread, 1);
		return -1;
	}

	ASSERT(INTR_OFF == intr_get_status() && parent_thread->pgdir != NULL);
	if (copy_process(child_thread, parent_thread)) {
		return -1;
	}

	ASSERT(!elem_find(&thread_ready_list, &child_thread->general_tag));
	list_append(&thread_ready_list, &child_thread->general_tag);	
	ASSERT(!elem_find(&thread_all_list, &child_thread->all_list_tag));
	list_append(&thread_all_list, &child_thread->all_list_tag);

	return child_thread->pid;
}
