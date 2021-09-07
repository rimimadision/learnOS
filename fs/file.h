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

#endif //__FS_FILE_H
