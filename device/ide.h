#ifndef __DEVICE_IDE_H
#define __DEVICE_IDE_H

#include "stdint.h"
#include "list.h"
#include "bitmap.h"
#include "sync.h"

#define CHANNEL_CNT 2
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

struct ide_channel {
	char name[8];
	uint16_t port_base;
	uint8_t irq_no;
	struct lock lock;
	bool expecting_intr; // true for waiting interrupt form disk
	struct semaphore disk_done; // 0 for block itself after send command to disk
	struct disk devices[2];
};

void ide_init(void);
void ide_read(struct disk* hd, uint32_t lba, void* buf, uint32_t sec_cnt);
void ide_write(struct disk* hd, uint32_t lba, void* buf, uint32_t sec_cnt);
void intr_hd_handler(uint8_t irq_no);

#endif // __DEVICE_IDE_H
