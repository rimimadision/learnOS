#ifndef __FS_FILE_H
#define __FS_FILE_H

#include "stdint.h"
#include "inode.h"
#include "fs.h"
#include "dir.h"

#define MAX_FILE_OPEN 32
#define MAX_FILE_SECTORS (12 + (512 / 4))
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
	enum oflags fd_flags;
	struct inode* fd_inode;
};

extern struct file file_table[MAX_FILE_OPEN]; // 0, 1, 2 are taken by std_io

int32_t get_free_slot_in_global(void);
int32_t pcb_fd_install(int32_t global_fd_idx);
int32_t inode_bitmap_alloc(struct partition* part);
int32_t block_bitmap_alloc(struct partition* part);
void bitmap_sync(struct partition* part, uint32_t bit_idx, enum bitmap_type btmp);
int32_t file_create(struct dir* parent_dir, char* filename, enum oflags flag);
int32_t file_open(uint32_t inode_no, enum oflags flag);
int32_t file_close(struct file* file);
int32_t file_write(struct file* file, const void* buf, uint32_t count);

#endif //__FS_FILE_H
