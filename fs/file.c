#include "file.h"
#include "stdint.h"

struct file file_table[MAX_FILE_OPEN]; // 0, 1, 2 are taken by std_io

/* get a free slot from file table
 * return index if success, -1 if fail
 */
int32_t get_free_slot_in_global() {
	uint32_t fd_idx = 3;
	while (fd_idx < MAX_FILE_OPEN) {
		if (file_table[fd_idx].fd_inode == NULL) {
			break;
		}
		fd_idx++;
	}

	if (fd_idx == MAX_FILE_OPEN) {
		printk("exceed max open files\n");
		return -1;
	}
	return fd_idx;
}
