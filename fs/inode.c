#include "inode.h"
#include "global.h"
#include "ide.h"
#include "string.h"
#include "list.h"
#include "thread.h"
#include "memory.h"
#include "debug.h"
#include "interrupt.h"

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
	
	ASSERT(inode_pos->sec_lba < part->sb->data_start_lba);
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

	while (elem != part->open_inode.tail) {
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
