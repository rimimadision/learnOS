#ifndef __FS_INODE_H
#define __FS_INODE_H

#include "stdint.h"
#include "global.h"

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

#endif // __FS_INODE_H
