#ifndef __FS_FS_H
#define __FS_FS_H

#define MAX_FILES_PER_PART 4096
#define BITS_PER_SECTOR 4096
#define SECTOR_SIZE 512 
#define BLOCK_SIZE SECTOR_SIZE

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

extern partition* cur_part;

void filesys_init(void);

#endif // __FS_FS_H
