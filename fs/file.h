#ifndef __FS_FILE_H
#define __FS_FILE_H

#include "stdint.h"
#include "inode.h"
#include "fs.h"

#define MAX_FILE_OPEN 32

enum std_fd {
	stdin_no,
	stdout_no,
	stderr_no	
};

enum bitmap_type {
	INODE_BITMAP,
	BLOCK_BITMAP
};

struct file { // file struct in global file table 
	uint32_t fd_pos;
	enum oflgs fd_flags;
	struct inode* fd_inode;
};

int32_t get_free_slot_in_global(void);
int32_t pcb_fd_install(int32_t global_fd_idx);
int32_t inode_bitmap_alloc(struct partition* part);
int32_t block_bitmap_alloc(struct partition* part);
void bitmap_sync(struct partition* part, uint32_t bit_idx, enum bitmap_type btmp);

#endif //__FS_FILE_H
