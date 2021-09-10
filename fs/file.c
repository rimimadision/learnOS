#include "file.h"
#include "stdint.h"
#include "thread.h"
#include "stdio-kernel.h"
#include "ide.h"
#include "bitmap.h"
#include "interrupt.h"
#include "super_block.h"
#include "dir.h"
#include "memory.h"
#include "global.h"
#include "inode.h"

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

int32_t file_create(struct dir* parent_dir, char* filename, enum oflags flag) {
	void* io_buf = sys_malloc(1024);
	if (io_buf == NULL) {
		printk("in file_create:sys_malloc for io_buf failed\n");
		return -1;
	}

	uint8_t rollback_step = 0;
	
	/* alloc inode for new file */
	int32_t inode_no = inode_bitmap_alloc(cur_part);
	if (inode_no == -1) {
		printk("in file_create:alloc inode failed\n");
		return -1;
	}	
	
	enum intr_status intr_old_status = intr_disable();	
	struct task_struct* cur = get_cur_thread_pcb(); 
	uint32_t* cur_pgdir = cur->pgdir;
	cur->pgdir= NULL;
	struct inode* new_file_inode = (struct inode*)sys_malloc(sizeof(struct inode));
	cur->pgdir = cur_pgdir;
	intr_set_status(intr_old_status);

	if (new_file_inode == NULL) {
		printk("file_create:sys_malloc for inode failed\n");
		rollback_step = 1;	
		goto rollback;
	}

	inode_init(inode_no, new_file_inode);

	/* find free slot in file_table */
	int fd_idx = get_free_slot_in_global();

	if (fd_idx == -1) {
		printk("exceed max open file\n");
		rollback_step = 2;
		goto rollback;
	} 

	file_table[fd_idx].fd_inode = new_file_inode;
	file_table[fd_idx].fd_pos = 0;
	file_table[fd_idx].fd_flags = flag;	
	file_table[fd_idx].fd_inode->write_deny = false;

	struct dir_entry new_dir_entry;
	memset(&new_dir_entry, 0, sizeof(struct dir_entry));

	create_dir_entry(filename, inode_no, FT_REGULAR, &new_dir_entry);
	
	if(!sync_dir_entry(parent_dir, &new_dir_entry, io_buf)) {
		printk("sync dir_entry to disk failed\n");
		rollback_step = 3;
		goto rollback;
	}

	memset(io_buf, 0, 1024);
	inode_sync(cur_part, parent_dir->inode, io_buf);
	memset(io_buf, 0, 1024);
	inode_sync(cur_part, new_file_inode, io_buf);
	bitmap_sync(cur_part, inode_no, INODE_BITMAP);
	list_push(&cur_part->open_inode, &new_file_inode->inode_tag);
	new_file_inode->i_open_cnts = 1;

	sys_free(io_buf);
	return pcb_fd_install(fd_idx);

rollback:
	switch (rollback_step) {
		case 3: 
			memset(&file_table[fd_idx], 0, sizeof(struct file));
		case 2:
			intr_old_status = intr_disable();	
			cur = get_cur_thread_pcb(); 
			cur_pgdir = cur->pgdir;
			cur->pgdir= NULL;
			sys_free(new_file_inode);
			cur->pgdir = cur_pgdir;
			intr_set_status(intr_old_status);
		case 1:
			bitmap_set(&cur_part->inode_bitmap, inode_no, 0);
			break;
	}

	sys_free(io_buf);
	return -1;
}

int32_t file_open(uint32_t inode_no, enum oflags flag) {
	int fd_idx = get_free_slot_in_global();
	if (fd_idx == -1) {
		printk("exceed max open file\n");
		return -1;
	}

	file_table[fd_idx].fd_inode = inode_open(cur_part, inode_no);
	file_table[fd_idx].fd_pos = 0;
	file_table[fd_idx].fd_flags = flag;

	bool* write_deny = &file_table[fd_idx].fd_inode->write_deny;
	if (flag & O_WRONLY || flag & O_RDWR) {
		enum intr_status old_status = intr_disable();
		if (!(*write_deny)) {
			*write_deny = true;
			intr_set_status(old_status);
		} else {
			intr_set_status(old_status);
			printk("file can't be write now, try again later\n");
			return -1;
		}
	}

	return pcb_fd_install(fd_idx);
}

int32_t file_close(struct file* file) {
	if (file == NULL) {
		return -1;
	}

	file->fd_inode->write_deny = false;
	inode_close(file->fd_inode);
	file->fd_inode = NULL;
	return 0;
}

int32_t file_write(struct file* file, const void* buf, uint32_t count) {
	if (()) {

	}
}
