#ifndef __FS_FS_H
#define __FS_FS_H

#include "stdint.h"

#define MAX_FILES_PER_PART 4096
#define BITS_PER_SECTOR 4096
#define SECTOR_SIZE 512 
#define BLOCK_SIZE SECTOR_SIZE
#define MAX_PATH_LEN 512

enum file_types {
	FT_UNKNOWN,
	FT_REGULAR,
	FT_DIRECTORY
};

enum oflags {
	O_RDONLY,
	O_WRONLY,
	O_RDWR,
	O_CREAT = 4
};

struct path_search_record{
	char searched_path[MAX_PATH_LEN];
	struct dir* parent_dir;
	enum file_types file_type;
};

extern struct partition* cur_part;

void filesys_init(void);
int32_t path_depth_cnt(char* pathname);
int32_t sys_open(const char* pathname, uint8_t flags);
int32_t sys_close(int32_t fd);
int32_t sys_write(int32_t fd, const void* buf, uint32_t count);

#endif // __FS_FS_H
