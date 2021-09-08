#include "file.h"
#include "stdint.h"
#include "thread.h"
#include "stdio-kernel.h"
#include "ide.h"
#include "bitmap.h"
#include "interrupt.h"
#include "super_block.h"

struct file file_table[MAX_FILE_OPEN]; // 0, 1, 2 are taken by std_io

/* get a free slot from file table
 * return index if success, -1 if fail
 */
int32_t get_free_slot_in_global(void) {
	uint32_t fd_idx = 3;
	while (fd_idx < MAX_FILE_OPEN) {
		if (file_table[fd_idx].fd_inode == NULL) {
			break;
		}
		fd_idx++;
	}

	if (fd_idx == MAX_FILE_OPEN) {
		printk("exceed max open files\n");
		return -1;
	}
	return fd_idx;
}

int32_t pcb_fd_install(int32_t global_fd_idx) {
	struct task_struct* cur = get_cur_thread_pcb();
	uint8_t local_fd_idx = 3;	
	while (local_fd_idx < MAX_FILES_OPEN_PER_PROC) {
		if (cur->fd_table[local_fd_idx] == -1) {
			cur->fd_table[local_fd_idx] = global_fd_idx;
			break;
		} 
		local_fd_idx++;
	}
	
	if (local_fd_idx == MAX_FILES_OPEN_PER_PROC) {
		printk("exceed max open files_per_proc\n");
		return -1;
	}

	return local_fd_idx;
}

/* return inode no */
int32_t inode_bitmap_alloc(struct partition* part) {
	enum intr_status old_status = intr_disable();	
	
	int32_t bit_idx = bitmap_scan(&part->inode_bitmap, 1);
	if (bit_idx == -1) {
		return -1;
	}
	bitmap_set(&part->inode_bitmap, bit_idx, 1);
	intr_set_status(old_status);

	return bit_idx;
}	

/* return block_lba */
int32_t block_bitmap_alloc(struct partition* part) {
	enum intr_status old_status = intr_disable();	
	
	int32_t bit_idx = bitmap_scan(&part->block_bitmap, 1);
	if (bit_idx == -1) {
		return -1;
	}
	bitmap_set(&part->block_bitmap, bit_idx, 1);
	intr_set_status(old_status);

	return (part->sb->data_start_lba + bit_idx);
}

void bitmap_sync(struct partition* part, uint32_t bit_idx, enum bitmap_type btmp) {
	uint32_t off_sec = bit_idx / (512 * 8);
	uint32_t off_size = off_sec * BLOCK_SIZE;
	uint32_t sec_lba;
	uint8_t* bitmap_off;

	switch (btmp) {
		case INODE_BITMAP:
			sec_lba = part->sb->inode_bitmap_lba + off_sec;
			bitmap_off = part->inode_bitmap.btmp_addr + off_size;
			break;
		case BLOCK_BITMAP:
			sec_lba = part->sb->block_bitmap_lba + off_sec;
			bitmap_off = part->block_bitmap.btmp_addr + off_size;
			break;
	}

	ide_write(part->my_disk, sec_lba, bitmap_off, 1);
} 
