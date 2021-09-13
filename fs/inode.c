#include "inode.h"
#include "bitmap.h"
#include "stdint.h"
#include "global.h"
#include "ide.h"
#include "string.h"
#include "list.h"
#include "thread.h"
#include "memory.h"
#include "debug.h"
#include "interrupt.h"
#include "super_block.h"
#include "file.h"

struct inode_position {
	bool two_sec;
	uint32_t sec_lba; // sector where inode lies
	uint32_t off_size; // inode offset in a sector
};

static void inode_locate(struct partition* part, uint32_t inode_no,\
	                     struct inode_position* inode_pos);

static void inode_locate(struct partition* part, uint32_t inode_no,\
	                     struct inode_position* inode_pos) {
	ASSERT(inode_no < 4096);
	uint32_t inode_table_lba = part->sb->inode_table_lba;
	uint32_t inode_size = sizeof(struct inode);
	
	uint32_t off_size_in_inode_table = inode_size * inode_no;
	inode_pos->sec_lba = off_size_in_inode_table / 512 + inode_table_lba;
	inode_pos->off_size = off_size_in_inode_table % 512;
	if (inode_pos->off_size > (512 - inode_size)) {
		inode_pos->two_sec = true;
	} else {
		inode_pos->two_sec = false;
	}	
}

void inode_sync(struct partition* part, struct inode* inode, void* io_buf) {
	uint8_t inode_no = inode->i_no;
	struct inode_position inode_pos;	
	inode_locate(part, inode_no, &inode_pos);
	
	ASSERT(inode_pos.sec_lba < part->sb->data_start_lba);
	// inode_tag and i_open_cnts will be reset after the file being loaded
	// so we can choose to clear them when we save inode or reset when we load
	struct inode pure_inode;
	memcpy(&pure_inode, inode, sizeof(struct inode)); 
	pure_inode.inode_tag.prev = pure_inode.inode_tag.next = NULL;
	pure_inode.i_open_cnts = 0;
	pure_inode.write_deny = false; // ensure we can write it after next reload
	
	char* inode_buf = (char*)io_buf;
	if (inode_pos.two_sec) { // read two sectors
		ide_read(part->my_disk, inode_pos.sec_lba, io_buf, 2);
		memcpy((inode_buf + inode_pos.off_size), &pure_inode, sizeof(struct inode));
		ide_write(part->my_disk, inode_pos.sec_lba, io_buf, 2);
	} else {
		ide_read(part->my_disk, inode_pos.sec_lba, io_buf, 1);
		memcpy((inode_buf + inode_pos.off_size), &pure_inode, sizeof(struct inode));
		ide_write(part->my_disk, inode_pos.sec_lba, io_buf, 1);
	}
}

struct inode* inode_open (struct partition* part, uint32_t inode_no) {
	struct list_elem* elem = part->open_inode.head.next;
	struct inode* inode_found;

	while (elem != &part->open_inode.tail) {
		inode_found = elem2entry(struct inode, inode_tag, elem);
		if (inode_found->i_no == inode_no) {
			inode_found->i_open_cnts++;
			return inode_found;
		}
		elem = elem->next;
	} 

	struct inode_position inode_pos;	
	inode_locate(part, inode_no, &inode_pos);
	
	/* we need to put inode in public kernel space
	   so we need to set PCB->page_dir NULL temporarily 
	 */
	enum intr_status intr_old_status = intr_disable();	
	struct task_struct* cur = get_cur_thread_pcb(); 
	uint32_t* cur_pgdir = cur->pgdir;
	cur->pgdir= NULL;
	inode_found = (struct inode*)sys_malloc(sizeof(struct inode));
	cur->pgdir = cur_pgdir;
	intr_set_status(intr_old_status);

	char* inode_buf;
	if (inode_pos.two_sec) {
		inode_buf = (char*)sys_malloc(1024);
		ide_read(part->my_disk, inode_pos.sec_lba, inode_buf, 2);
	} else {	
		inode_buf = (char*)sys_malloc(512);
		ide_read(part->my_disk, inode_pos.sec_lba, inode_buf, 1);
	}

	memcpy(inode_found, inode_buf + inode_pos.off_size, sizeof(struct inode));
	list_push(&part->open_inode, &inode_found->inode_tag);
	inode_found->i_open_cnts++;
	sys_free(inode_buf);
		
	return inode_found;	
}

void inode_close(struct inode* inode) {
	enum intr_status old_status = intr_disable();

	if(--inode->i_open_cnts == 0) {
		list_remove(&inode->inode_tag);
		struct task_struct* cur = get_cur_thread_pcb(); 
		uint32_t* cur_pgdir = cur->pgdir;
		cur->pgdir= NULL;
		sys_free(inode);
		cur->pgdir = cur_pgdir;
	}

	intr_set_status(old_status);
}

void inode_init(uint32_t inode_no, struct inode* new_inode) {
	new_inode->i_no = inode_no;
	new_inode->i_size = 0;
	new_inode->i_open_cnts = 0;
	new_inode->write_deny = false;

	uint8_t sec_idx = 0;
	while (sec_idx < 13) {
		new_inode->i_sectors[sec_idx] = 0;
		sec_idx++;
	}
}

void inode_delete(struct partition* part, uint32_t inode_no, void* io_buf) {
	ASSERT(inode_no < 4096);
	struct inode_position inode_pos;
	inode_locate(part, inode_no, &inode_pos);
	ASSERT(inode_pos.sec_lba <= part->sb->data_start_lba);

	char* inode_buf = (char*)io_buf;
	if (inode_pos.two_sec) {
		ide_read(part->my_disk, inode_pos.sec_lba, inode_buf, 2);
		memset((inode_buf + inode_pos.off_size), 0, sizeof(struct inode));
		ide_write(part->my_disk, inode_pos.sec_lba, inode_buf, 2);
	} else {
		ide_read(part->my_disk, inode_pos.sec_lba, inode_buf, 1);
		memset((inode_buf + inode_pos.off_size), 0, sizeof(struct inode));
		ide_write(part->my_disk, inode_pos.sec_lba, inode_buf, 1);
	}
}

void inode_release(struct partition* part, uint32_t inode_no) {
	struct inode* inode_to_del = inode_open(part, inode_no);
	ASSERT(inode_to_del->i_no == inode_no);
		
	uint8_t block_idx = 0, block_cnt = 12;
	uint32_t block_bitmap_idx;
	uint32_t all_block[140] = {0};

	while (block_idx < 12) {
		all_block[block_idx] = inode_to_del->i_sectors[block_idx];
		block_idx++;
	}

	if (inode_to_del->i_sectors[12] != 0) {
		ide_read(part->my_disk, inode_to_del->i_sectors[12], all_block + 12, 1);
		block_cnt = 140;
		/* free blocks taken by indirect blocks table */
		block_bitmap_idx = inode_to_del->i_sectors[12] - part->sb->data_start_lba;
		ASSERT(block_bitmap_idx > 0);
		bitmap_set(&part->block_bitmap, block_bitmap_idx, 0);
		bitmap_sync(cur_part, block_bitmap_idx, BLOCK_BITMAP);
	}
	
	block_idx = 0;
	while (block_idx < block_cnt) {
		if (all_block[block_idx] != 0) {
			block_bitmap_idx = 0;
			block_bitmap_idx = all_block[block_idx] - part->sb->data_start_lba;
			ASSERT(block_bitmap_idx > 0);
			bitmap_set(&part->block_bitmap, block_bitmap_idx, 0);
			bitmap_sync(cur_part, block_bitmap_idx, BLOCK_BITMAP);
		}
		block_idx++;
	}
	
	bitmap_set(&part->inode_bitmap, inode_no, 0);
	bitmap_sync(cur_part, inode_no, INODE_BITMAP);

	void* io_buf = sys_malloc(1024);
	inode_delete(part, inode_no, io_buf);
	sys_free(io_buf);
	
	inode_close(inode_to_del);
}
