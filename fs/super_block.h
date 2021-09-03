#ifndef __FS_SUPERBLOCK_H
#define __FS_SUPERBLOCK_H

#include "stdint.h"

struct super_block {
	uint32_t magic;

	uint32_t sec_cnt;
	uint32_t inode_cnt;
	uint32_t part_lba_base;

	uint32_t block_bitmap_lba;
	uint32_t block_bitmap_sects;

	uint32_t inode_bitmap_lba;
	uint32_t inode_bitmap_sects;

	uint32_t inode_table_lba;
	uint32_t inode_table_sects;

	uint32_t data_start_lba;
	uint32_t root_inode_no;
	uint32_t dir_entry_size;
	
	uint8_t pad[460];
}__attribute__((packed));

#endif // __FS_SUPERBLOCK_H
