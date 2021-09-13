#ifndef __FS_INODE_H
#define __FS_INODE_H

#include "stdint.h"
#include "global.h"
#include "list.h"
#include "ide.h"

struct inode {

	uint32_t i_no;
	uint32_t i_size; // if inode point a file, size means file size;
				   // if inode point a dir, size means total size 
				   // of its entry
	uint32_t i_open_cnts;
	bool write_deny;
	uint32_t i_sectors[13]; // 0 - 11 is direct block pointer
						    // 12 is first-level indirect block pointer
	struct list_elem inode_tag;
};

void inode_sync(struct partition* part, struct inode* inode, void* io_buf);
struct inode* inode_open (struct partition* part, uint32_t inode_no);
void inode_close(struct inode* inode);
void inode_init(uint32_t inode_no, struct inode* new_inode);
void inode_delete(struct partition* part, uint32_t inode_no, void* io_buf);
void inode_release(struct partition* part, uint32_t inode_no);

#endif // __FS_INODE_H
