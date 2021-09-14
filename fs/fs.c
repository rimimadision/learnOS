#include "fs.h"
#include "stdint.h"
#include "global.h"
#include "inode.h"
#include "ide.h"
#include "super_block.h"
#include "dir.h"
#include "stdio-kernel.h"
#include "memory.h"
#include "string.h"
#include "list.h"
#include "debug.h"
#include "file.h"
#include "thread.h"
#include "console.h"

struct partition* cur_part;

static void partition_format(struct partition* part);
static bool mount_partition(struct list_elem* pelem, int arg);
static char* path_parse(char* pathname, char* name_store);
static uint32_t fd_local2global(uint32_t local_fd);

/* initialize partition, create fs in it */
static void partition_format(struct partition* part) {
	uint32_t boot_sector_sects = 1;
	uint32_t super_block_sects = 1;
	uint32_t inode_bitmap_sects = DIV_ROUND_UP(MAX_FILES_PER_PART, BITS_PER_SECTOR);
	uint32_t inode_table_sects = DIV_ROUND_UP(sizeof(struct inode) * MAX_FILES_PER_PART, SECTOR_SIZE);	
	
	uint32_t used_sects = boot_sector_sects + super_block_sects + inode_bitmap_sects + inode_table_sects;
	uint32_t free_sects = part->sec_cnt - used_sects;
	
	uint32_t block_bitmap_sects;
	block_bitmap_sects = DIV_ROUND_UP(free_sects, BITS_PER_SECTOR); 
	uint32_t block_bitmap_bit_len = free_sects - block_bitmap_sects;
	block_bitmap_sects = DIV_ROUND_UP(block_bitmap_bit_len, BITS_PER_SECTOR); 	
	
	struct super_block sb;
	sb.magic = 0x19700505;
	sb.sec_cnt = part->sec_cnt;
	sb.inode_cnt = MAX_FILES_PER_PART;
	sb.part_lba_base = part->start_lba;
	sb.block_bitmap_lba = sb.part_lba_base + boot_sector_sects + super_block_sects;
	sb.block_bitmap_sects = block_bitmap_sects;
	sb.inode_bitmap_lba = sb.block_bitmap_lba + sb.block_bitmap_sects;
	sb.inode_bitmap_sects = inode_bitmap_sects;
	sb.inode_table_lba = sb.inode_bitmap_lba + sb.inode_bitmap_sects;	
	sb.inode_table_sects = inode_table_sects;
	sb.data_start_lba = sb.inode_table_sects + sb.inode_table_lba;
	sb.root_inode_no = 0;
	sb.dir_entry_size = sizeof(struct dir_entry);

	printk("\n%s info\n", part->name);
	printk("    magic:0x%x\n    part_lba_base:0x%x\n    all_sectors:0x%x\n    inode_cnt:0x%x\n"\
           "    block_bitmap_lba:0x%x\n    block_bitmap_sectors:0x%x\n    inode_bitmap_lba:0x%x\n"\
           "    inode_bitmap_sectors:0x%x\n    inode_table_lba:0x%x\n    inode_table_sectors:0x%x\n"\
           "    data_start_lba:0x%x\n", sb.magic, sb.part_lba_base, sb.sec_cnt, sb.inode_cnt,\
		   sb.block_bitmap_lba, sb.block_bitmap_sects, sb.inode_bitmap_lba, sb.inode_bitmap_sects,\
           sb.inode_table_lba, sb.inode_table_sects, sb.data_start_lba);

	struct disk* hd = part->my_disk;
	
	ide_write(hd, part->start_lba + 1, &sb, 1);
	printk("    super_block_lba:0x%x\n", part->start_lba + 1);

	uint32_t buf_size = (sb.block_bitmap_sects >= sb.inode_bitmap_sects) ? sb.block_bitmap_sects : sb.inode_bitmap_sects;	
	buf_size = ((buf_size >= sb.inode_table_sects) ? buf_size : sb.inode_table_sects) * SECTOR_SIZE;
	uint8_t* buf = sys_malloc(buf_size);
	
	buf[0] |= 0x01; // taken by root dir
	uint32_t block_bitmap_last_byte = block_bitmap_bit_len / 8;
	uint32_t block_bitmap_last_bit = block_bitmap_bit_len % 8;	
		 	
	uint32_t last_size = SECTOR_SIZE - (block_bitmap_last_byte % SECTOR_SIZE);
	memset(&buf[block_bitmap_last_byte], 0xff, last_size); // set the redundant byte 1

	// set last few bits in a byte 0
	uint32_t bit_idx = 0;
	while (bit_idx < block_bitmap_last_bit) {
		buf[block_bitmap_last_byte] &= (~(1 << bit_idx));
		bit_idx++;		
	}
	ide_write(hd, sb.block_bitmap_lba, buf, sb.block_bitmap_sects);

	memset(buf, 0, buf_size);
	buf[0] |= 0x01;
	ide_write(hd, sb.inode_bitmap_lba, buf, sb.inode_bitmap_sects);
	
	memset(buf, 0, buf_size);
	/* initialize the first entry 'root' in inode_table */
	struct inode* i = (struct inode*)buf;
	i->i_size = sb.dir_entry_size * 2; // . and ..
	i->i_no = 0;
	i->i_sectors[0] = sb.data_start_lba;
	ide_write(hd, sb.inode_table_lba, buf, sb.inode_table_sects);
	
	/* write root into first block in free blocks(start form data_start_lba)*/
	memset(buf, 0, buf_size);
	struct dir_entry* p_de = (struct dir_entry*)buf;
	/* init . */
	memcpy(p_de->filename, ".", 1);
	p_de->i_no = 0;
	p_de->f_type = FT_DIRECTORY;
	p_de++;
	
	/* init .. */
	memcpy(p_de->filename, "..", 2);
	p_de->i_no = 0; // root's father is still root 
	p_de->f_type = FT_DIRECTORY;

	ide_write(hd, sb.data_start_lba, buf, 1);
	
	printk("    root_dir_lba:0x%x\n", sb.data_start_lba);
	
	sys_free(buf);
	printk("%s format done\n", part->name);
}

void filesys_init(void) {
	uint8_t channel_no = 0, dev_no = 0, part_idx = 0;
	
	struct super_block* sb_buf = (struct super_block*)sys_malloc(SECTOR_SIZE);
	
	if (sb_buf == NULL) {
		PANIC("\nalloc memory failed\n");
	}
	printk("\nsearching filesystem......\n");
	while (channel_no < channel_cnt) {
		dev_no = 0;
	
		while (dev_no < 2) {
			part_idx = 0;
			if (dev_no == 0) {
				dev_no++;
				continue;
			}
			struct disk* hd = &channels[channel_no].devices[dev_no];
			struct partition* part = hd->prim_parts;
			while (part_idx < 12) {
				if (part_idx == 4) {
					part = hd->logic_parts;
				}
				
				if (part->sec_cnt != 0) {
					memset(sb_buf, 0, SECTOR_SIZE);
					ide_read(hd, part->start_lba + 1, sb_buf, 1);

					if (sb_buf->magic == 0x19700505) {
						printk("%s has filesystem\n", part->name);
					} else {
						printk("formatting %s's partition %s......\n", hd->name, part->name);
						partition_format(part);
					}
				}
				part_idx++;
				part++;
			}
			dev_no++;
		}
		channel_no++;
	}
	sys_free(sb_buf);

	char default_part[8] = "sdb1";
	list_traversal(&partition_list, mount_partition, (int)default_part);
	open_root_dir(cur_part);
	
	uint32_t fd_idx = 0;
	while (fd_idx < MAX_FILE_OPEN) {
		file_table[fd_idx++].fd_inode = NULL;
	}

	printk("data_start:%x\n", cur_part->sb->data_start_lba);
	printk("inode_table:%x\n", cur_part->sb->inode_table_lba);
	printk("inode_bitmap:%x\n", cur_part->sb->inode_bitmap_lba);
	printk("block_bitmap:%x\n", cur_part->sb->block_bitmap_lba);
	printk("inode_size:%x\n", sizeof(struct inode));
}

static bool mount_partition(struct list_elem* pelem, int arg) {
	char* part_name = (char*)arg;
	struct partition* part = elem2entry(struct partition, part_tag, pelem);	
	if (strcmp(part->name, part_name)) {
		return false;
	}

	cur_part = part;
	struct disk* hd = cur_part->my_disk;
	
	/* read super block */
	struct super_block* sb_buf = (struct super_block*)sys_malloc(SECTOR_SIZE);	
	cur_part->sb = (struct super_block*)sys_malloc(SECTOR_SIZE);
	if (sb_buf == NULL || cur_part->sb == NULL) {
		PANIC("alloc memory failed\n");
	}
	memset(sb_buf, 0, SECTOR_SIZE);
	ide_read(hd, cur_part->start_lba + 1, sb_buf, 1);
	memcpy(cur_part->sb, sb_buf, sizeof(struct super_block));	
	
	/* read block bitmap */
	cur_part->block_bitmap.btmp_addr =\
	(uint8_t*)sys_malloc(sb_buf->block_bitmap_sects * SECTOR_SIZE);
	if (cur_part->block_bitmap.btmp_addr == NULL) {
		PANIC("alloc memory failed\n");	
	}
	cur_part->block_bitmap.btmp_bytes_len = sb_buf->block_bitmap_sects * SECTOR_SIZE;
	ide_read(hd, sb_buf->block_bitmap_lba, cur_part->block_bitmap.btmp_addr, sb_buf->block_bitmap_sects);

	/* read inode bitmap */
	cur_part->inode_bitmap.btmp_addr =\
	(uint8_t*)sys_malloc(sb_buf->inode_bitmap_sects * SECTOR_SIZE);
	if (cur_part->inode_bitmap.btmp_addr == NULL) {
		PANIC("alloc memory failed\n");			
	}
	cur_part->inode_bitmap.btmp_bytes_len = sb_buf->inode_bitmap_sects * SECTOR_SIZE;
	ide_read(hd, sb_buf->inode_bitmap_lba, cur_part->inode_bitmap.btmp_addr, sb_buf->inode_bitmap_sects);

	list_init(&cur_part->open_inode);
	
	printk("mount %s done!\n", part->name);

	return true;
}

/* return the child pathname and store parent path in name_store */
static char* path_parse(char* pathname, char* name_store) {
	if (pathname[0] == '/') {
		while(*(++pathname) == '/');
	}

	while (*pathname != '/' && *pathname != '\0') {
		*name_store++ = *pathname++;
	}

	if (pathname[0] == '\0') {
		return NULL;
	}
	return pathname;
} 

int32_t path_depth_cnt(char* pathname) {
	ASSERT(pathname != NULL);
	char* p = pathname;
	char name[MAX_FILE_NAME_LEN];

	uint32_t depth = 0;
	p = path_parse(p, name);
	while (name[0]) {
		depth++;
		memset(name, 0, MAX_FILE_NAME_LEN);
		if (p) {
			p = path_parse(p, name);
		}
	}
	
	return depth;
}

/* return inode_no or -1 */
static int search_file(const char* pathname, struct path_search_record* searched_record) {
	if (!strcmp(pathname, "/") || !strcmp(pathname, "/.") || !strcmp(pathname, "/..")) {
		searched_record->parent_dir = &root_dir;
		searched_record->file_type = FT_DIRECTORY;
		searched_record->searched_path[0] = 0;

		return 0;
	}

	uint32_t path_len = strlen(pathname);
	ASSERT(pathname[0] == '/' && path_len > 1 && path_len < MAX_PATH_LEN);
	char* sub_path = (char*)pathname;
	struct dir* parent_dir = &root_dir;
	struct dir_entry dir_e;

	char name[MAX_FILE_NAME_LEN] = {0};
	searched_record->parent_dir = parent_dir;
	searched_record->file_type = FT_UNKNOWN;
	uint32_t parent_inode_no = 0;

	sub_path = path_parse(sub_path, name);

	while (name[0]) {
		ASSERT(strlen(searched_record->searched_path) < MAX_PATH_LEN);
		strcat(searched_record->searched_path, "/");
		strcat(searched_record->searched_path, name);

		if (search_dir_entry(cur_part, parent_dir, name, &dir_e)) {
			memset(name, 0, MAX_FILE_NAME_LEN);
			if (sub_path) {
				sub_path = path_parse(sub_path, name);
			}
			if(FT_DIRECTORY == dir_e.f_type) {
				parent_inode_no = parent_dir->inode->i_no;
				dir_close(parent_dir);
				parent_dir = dir_open(cur_part, dir_e.i_no);
				searched_record->parent_dir = parent_dir;
				continue;
			} else if (FT_REGULAR == dir_e.f_type) {
				searched_record->file_type = FT_REGULAR;
				return dir_e.i_no;
			}
		} else { return -1;} // not close parent_dir
	}
		
		/* only dir_name in path */
		dir_close(searched_record->parent_dir);
		searched_record->parent_dir = dir_open(cur_part, parent_inode_no);
		searched_record->file_type = FT_DIRECTORY;
	
		return dir_e.i_no;
}

int32_t sys_open(const char* pathname, uint8_t flags) {
	if (pathname[strlen(pathname) - 1] == '/') {
		printk("can't open a directory %s\n", pathname);
		return -1;
	}
	ASSERT(flags <= 7);
	int32_t fd = -1;

	struct path_search_record searched_record;
	memset(&searched_record, 0, sizeof(struct path_search_record));

	uint32_t pathname_depth = path_depth_cnt((char*)pathname);
	
	int inode_no = search_file(pathname, &searched_record);	
	bool found = inode_no != -1 ? true : false;

	if (searched_record.file_type == FT_DIRECTORY) {
		printk("can't open a directory with open(), use opendir() to instead\n");
		dir_close(searched_record.parent_dir);	
		return -1;
	}

	uint32_t path_searched_depth = path_depth_cnt(searched_record.searched_path);
	
	if (pathname_depth != path_searched_depth) {
		printk("cannot access %s:Not a directory, subpath %s is't exists\n", pathname, searched_record.searched_path);
		dir_close(searched_record.parent_dir);
		return -1;
	}

	if (!found && !(flags & O_CREAT)) {
		printk("in path %s, file %s is't exist\n", searched_record.searched_path, \
				(strrchr(searched_record.searched_path, '/') + 1));
		dir_close(searched_record.parent_dir);
		return -1;
	} else if (found && (flags & O_CREAT)) {
		printk("%s has already exsits\n", pathname);
		dir_close(searched_record.parent_dir);
		return -1;
	}

	switch (flags & O_CREAT) {
		case O_CREAT:
			printk("creating file\n");
			fd = file_create(searched_record.parent_dir, (strrchr(pathname, '/') + 1), flags);
			dir_close(searched_record.parent_dir);
			break;
		default:
			fd = file_open(inode_no, flags);
			break;
	}

	return fd;
}

static uint32_t fd_local2global(uint32_t local_fd) {
	struct task_struct* cur = get_cur_thread_pcb();
	int32_t global_fd = cur->fd_table[local_fd];
	ASSERT(global_fd >= 0 && global_fd < MAX_FILE_OPEN);
	return (uint32_t)global_fd;
} 

int32_t sys_close(int32_t fd) {
	int32_t ret = -1;
	if (fd > 2) {
		uint32_t _fd = fd_local2global(fd);
		ret = file_close(&file_table[_fd]);
		get_cur_thread_pcb()->fd_table[fd] = -1;
	}
	return ret;
}

int32_t sys_write(int32_t fd, const void* buf, uint32_t count) {
	if (fd < 0) {
		printk("sys_write:fd error\n");
		return -1;
	}

	if (fd == stdout_no) {
		char tmp_buf[1024] = {0};
		memcpy(tmp_buf, buf, count);
		console_put_str(tmp_buf);	
		return count;
	}

	if (fd == stdin_no) {return 0;}
	if (fd == stderr_no) {return 0;}

	uint32_t _fd = fd_local2global(fd);
	struct file* wr_file = &file_table[_fd];
	if (wr_file->fd_flags & O_WRONLY || wr_file->fd_flags & O_RDWR) {
		uint32_t bytes_written = file_write(wr_file, buf, count);
		return bytes_written;
	} else {
		console_put_str("sys_write:not allowed to write file without flag O_RDWR or O_WRONLY\n");
		return -1;
	}
}

int32_t sys_read(int32_t fd, void* buf, uint32_t count) {
	if (fd < 0) {
		printk("sys_read:fd error\n");
		return -1;		
	} 

	ASSERT(buf != NULL);
	uint32_t _fd = fd_local2global(fd);
	return file_read(&file_table[_fd], buf, count);
}

int32_t sys_lseek(int32_t fd, int32_t offset, uint8_t whence) {
	if (fd < 0) {
		printk("sys_lseek:fd error\n");
		return -1;
	}

	ASSERT(whence >= 1 && whence <= 3);
	uint32_t _fd = fd_local2global(fd);
	struct file* pf = &file_table[_fd];
	int32_t new_pos = 0;
	int32_t file_size = (int32_t)pf->fd_inode->i_size;
	switch (whence) {
		case SEEK_SET:
			new_pos = offset;	
			break;
		case SEEK_CUR:
			new_pos = (int32_t)pf->fd_pos + offset;
			break;
		case SEEK_END:
			new_pos = file_size + offset;
			break;
	}

	if (new_pos < 0 || new_pos > (file_size - 1)) {
		return -1;
	}
	
	return pf->fd_pos = new_pos;
}

int32_t sys_unlink(const char* pathname) {
	ASSERT(strlen(pathname) < MAX_FILE_NAME_LEN);	
	
	struct path_search_record searched_record;
	memset(&searched_record, 0, sizeof(struct path_search_record));
	int inode_no = search_file(pathname, &searched_record);
	ASSERT(inode_no != 0);
	if (inode_no == -1) {
		printk("file %s not found\n", pathname);
		dir_close(searched_record.parent_dir);
		return -1;
	}	

	if (searched_record.file_type == FT_DIRECTORY) {
		printk("can't delete a directory with unlink()\n");
		dir_close(searched_record.parent_dir);
		return -1;
	}

	/* check out if file is being openning */
	uint32_t file_idx = 0;
	while (file_idx < MAX_FILE_OPEN) {
		if (file_table[file_idx].fd_inode != NULL && file_table[file_idx].fd_inode->i_no == (uint32_t)inode_no) {
			break;
		}
		file_idx++;
	}

	if (file_idx < MAX_FILE_OPEN) {
		dir_close(searched_record.parent_dir);
		printk("file %s is in use, not allow to delete\n", pathname);
		return -1;
	}

	ASSERT(file_idx == MAX_FILE_OPEN);
	void* io_buf = sys_malloc(SECTOR_SIZE * 2);
	if (io_buf == NULL) {
		dir_close(searched_record.parent_dir);
		printk("sys_unlink:malloc for io_buf failed\n");
		return -1;
	}
	struct dir* parent_dir = searched_record.parent_dir;
	delete_dir_entry(cur_part, parent_dir, inode_no, io_buf);	
	inode_release(cur_part, inode_no);
	sys_free(io_buf);
	dir_close(searched_record.parent_dir);
	return 0;
}

int32_t sys_mkdir(const char* pathname) {
	uint8_t rollback_step = 0;
	void* io_buf = sys_malloc(SECTOR_SIZE * 2);	
	
	if (io_buf == NULL) {
		printk("sys_mkdir:malloc for io_buf failed\n");
		return -1;
	}

	struct path_search_record searched_record;
	memset(&searched_record, 0, sizeof(struct path_search_record));
	int inode_no = search_file(pathname, &searched_record);
	if (inode_no != -1) {
		printk("sys_mkdir:file or directory %s exist\n", pathname);
		rollback_step = 1;
		goto rollback;
	} else {
		uint32_t pathname_depth = path_depth_cnt((char*)pathname);
		uint32_t path_searched_depth = path_depth_cnt(searched_record.searched_path);
		if (pathname_depth != path_searched_depth) {
			printk("sys_mkdir:cannot access %s:not a directory, subpath %s isn't exist\n", pathname, searched_record.searched_path);
			rollback_step = 1;
			goto rollback;
		} 
	}	
	
	struct dir* parent_dir = searched_record.parent_dir;
	char* dirname = strrchr(searched_record.searched_path, '/') + 1;
	
	inode_no = inode_bitmap_alloc(cur_part);
	if (inode_no == -1) {
		printk("sys_mkdir:alloc inode failed\n");
		rollback_step = 1;
		goto rollback;
	}	
	struct inode new_dir_inode;
	inode_init(inode_no, &new_dir_inode);
	
	uint32_t block_bitmap_idx = 0;
	int32_t block_lba = -1;
	block_lba = block_bitmap_alloc(cur_part);
	if (block_lba == -1) {
		printk("sys_mkdir:block_bitmap_alloc for create directory failed\n");
		rollback_step = 2;
		goto rollback;
	}
	new_dir_inode.i_sectors[0] = block_lba;
	block_bitmap_idx = block_lba - cur_part->sb->data_start_lba;
	ASSERT(block_bitmap_idx != 0);
	bitmap_sync(cur_part, block_bitmap_idx, BLOCK_BITMAP);

	memset(io_buf, 0, SECTOR_SIZE * 2);
	struct dir_entry* p_de = (struct dir_entry*)io_buf;
	memcpy(p_de->filename, ".", 1);
	p_de->i_no =  inode_no;
	p_de->f_type = FT_DIRECTORY;
	p_de++;	
	memcpy(p_de->filename, "..", 2);
	p_de->i_no = parent_dir->inode->i_no;
	p_de->f_type = FT_DIRECTORY;
	ide_write(cur_part->my_disk, new_dir_inode.i_sectors[0], io_buf, 1);

	new_dir_inode.i_size = 2 * cur_part->sb->dir_entry_size;
	
	/* add to parent directory */
	struct dir_entry new_dir_entry;
	memset(&new_dir_entry, 0, sizeof(struct dir_entry));
	create_dir_entry(dirname, inode_no, FT_DIRECTORY, &new_dir_entry);
	memset(io_buf, 0, SECTOR_SIZE * 2);
	if (!sync_dir_entry(parent_dir, &new_dir_entry, io_buf)) {
		printk("sys_mkdir:sync_dir_entry to disk failed\n");
		rollback_step = 2;
		goto rollback;
	}

	memset(io_buf, 0, SECTOR_SIZE * 2);
	inode_sync(cur_part, parent_dir->inode, io_buf);
	memset(io_buf, 0, SECTOR_SIZE * 2);
	inode_sync(cur_part, &new_dir_inode, io_buf);

	bitmap_sync(cur_part, inode_no, INODE_BITMAP);
	
	sys_free(io_buf);
	dir_close(searched_record.parent_dir);
	return 0;

rollback:
	switch (rollback_step) {
		case 2:
			bitmap_set(&cur_part->inode_bitmap, inode_no, 0);
		case 1:
			dir_close(searched_record.parent_dir);
			break;
}
	sys_free(io_buf);
	return -1;
}

struct dir* sys_opendir(const char* name) {
	ASSERT(strlen(name) < MAX_PATH_LEN);
	
	/* check if root_dir */
	if (name[0] == '/' && name[1] == 0) {
		return &root_dir;
	}

	
	
}
