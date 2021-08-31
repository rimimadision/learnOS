#ifndef __DEVICE_IDE_H
#define __DEVICE_IDE_H

#include "stdint.h"
#include "list.h"
#include "bitmap.*h"

#define MAX_LOGIC_PART 8 // support 8 logic partition at most

struct partition{
	uint32_t start_lba;
	uint32_t sec_cnt;
	struct disk* my_disk;
	struct list_elem part_tag;
	char name[8];
	struct super_block* sb;
	struct bitmap block_bitmap; 	
	struct bitmap inode_bitmap;
	struct list open_inode;
};

struct disk {
	char name[8];
	struct ide_channel* my_channel;
	uint8_t dev_no; // 0 for master, 1 for slave
	struct partition prim_parts[4];
	struct partition logic_parts[MAX_LOGIC_PART];
};

#endif // __DEVICE_IDE_H
