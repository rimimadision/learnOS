#include "ide.h"
#include "global.h"
#include "stdio.h"
#include "stdio-kernel.h"
#include "io.h"
#include "debug.h"
#include "timer.h"
#include "sync.h"
#include "interrupt.h"
#include "string.h"
#include "list.h"

/* define some ports number about disk */
#define reg_data(channel) (channel->port_base + 0)
#define reg_error(channel) (channel->port_base + 1)
#define reg_sect_cnt(channel) (channel->port_base + 2)
#define reg_lba_l(channel) (channel->port_base + 3)
#define reg_lba_m(channel) (channel->port_base + 4)
#define reg_lba_h(channel) (channel->port_base + 5)
#define reg_dev(channel) (channel->port_base + 6)
#define reg_status(channel) (channel->port_base + 7)
#define reg_cmd(channel) (reg_status(channel))
#define reg_alt_status(channel) (channel->port_base + 0x206)
#define reg_ctl(channel) (reg_alt_status(channel))

/* useful bit in reg_alt_status */
#define BIT_ALT_STAT_BSY (1 << 7)
#define BIT_ALT_STAT_DRDY (1 << 6)
#define BIT_ALT_STAT_DRQ (1 << 3)
#define BIT_ALT_STAT_ERR (1 << 0)

/* useful bit in reg_dev */
#define BIT_DEV_MBS ((1 << 7) + (1 << 5))
#define BIT_DEV_MOD_LBA (1 << 6)
#define BIT_DEV_DEV_MSR (0 << 4)
#define BIT_DEV_DEV_SLV (1 << 4)

/* useful command */
#define CMD_IDENTIFY 0xec
#define CMD_READ_SECTOR 0x20
#define CMD_WRITE_SECTOR 0x30

/* constants */
#define max_lba ((80 * 1024 * 1024 / 512) - 1)
uint8_t channel_cnt; // counted by disks_cnt

/* global var used by partition_scan */
uint32_t ext_lba_base = 0;
uint8_t p_no = 0, l_no = 0;
struct list partition_list;

struct partition_table_entry {
	uint8_t bootable;
	uint8_t start_head;
	uint8_t start_sec;
	uint8_t start_chs;
	uint8_t fs_type;
	uint8_t end_head;
	uint8_t end_sec;
	uint8_t end_chs;

	/* important two entry */
	uint32_t start_lba;
	uint32_t sec_cnt;
}__attribute__((packed)); // ensure the struct is 16 bytes

struct boot_sector {
	uint8_t other[446];
	struct partition_table_entry partition_table[4];
	uint16_t signature; // 0xaa55
}__attribute__((packed));

struct ide_channel channels[CHANNEL_CNT];

static void select_disk(struct disk* hd);
static void select_sector(struct disk* hd, uint32_t lba, uint8_t sec_cnt);
static void cmd_out(struct ide_channel* channel, uint8_t cmd);
static void read_from_sector(struct disk* hd, void* buf, uint8_t sec_cnt);
static void write2sector(struct disk* hd, void* buf, uint8_t sec_cnt);
static void swap_pairs_bytes(const char* dst, char* buf, uint32_t len);
static void identity_disk(struct disk* hd);
static bool partition_info(struct list_elem* pelem, int arg UNUSED);
static void partition_scan(struct disk* hd, uint32_t ext_lba);

void ide_init(void) {
	printk("ide_init start\n");
	
	uint8_t hd_cnt = *((uint8_t*)(0x475));
	channel_cnt = DIV_ROUND_UP(hd_cnt, 2); // I don't know how to get channel used by computer
	struct ide_channel* channel;
	uint8_t channel_no = 0, dev_no = 0;
	list_init(&partition_list);
	ASSERT(hd_cnt == 2 && channel_cnt == 1);

	while(channel_no < channel_cnt) {
		channel = &channels[channel_no];		
		sprintf(channel->name, "ide%d", channel_no);
		switch(channel_no) {
			case 0:
				channel->port_base = 0x1f0;	
				channel->irq_no = 0x20 + 14;
				break;
			case 1:
				channel->port_base = 0x170;
				channel->irq_no = 0x20 + 15;
				break;
		}	
		
		channel->expecting_intr = false;
		lock_init(&channel->lock);
		sema_init(&channel->disk_done, 0);
		register_handler(channel->irq_no, intr_hd_handler);

		while(dev_no < 2) {
			struct disk* hd = &channel->devices[dev_no];
			memset(hd, 0, sizeof(struct disk));
			hd->my_channel = channel;
			hd->dev_no = dev_no;
			sprintf(hd->name, "sd%c", 'a' + channel_no * 2 + dev_no);
			identity_disk(hd);
			if (dev_no != 0) {
				partition_scan(hd, 0);
			}
			p_no = 0, l_no = 0;
			dev_no++;
		}
		dev_no = 0;
		channel_no ++;
	}

	printk("\nall partition info\n");
	list_traversal(&partition_list, partition_info, (int)NULL);

	printk("ide_init done\n");
}

static void select_disk(struct disk* hd) {
	uint8_t reg_device = BIT_DEV_MBS | BIT_DEV_MOD_LBA;
	if(hd->dev_no == 1) {
		reg_device |= BIT_DEV_DEV_SLV;
	}

	outb(reg_dev(hd->my_channel), reg_device);
}

static void select_sector(struct disk* hd, uint32_t lba, uint8_t sec_cnt) {
	ASSERT(lba <= max_lba && (lba >> 28) == 0);
	struct ide_channel* channel = hd->my_channel;
	outb(reg_sect_cnt(channel), sec_cnt); // when sec_cnt == 0, means operate 256 sectors
	outb(reg_lba_l(channel), (uint8_t)(lba & 0xff));
	outb(reg_lba_m(channel), (uint8_t)((lba >> 8) & 0xff));
	outb(reg_lba_h(channel), (uint8_t)((lba >> 16) & 0xff));
	outb(reg_dev(channel), BIT_DEV_MBS | BIT_DEV_MOD_LBA |\
		 ((hd->dev_no == 0) ? BIT_DEV_DEV_MSR : BIT_DEV_DEV_SLV) |\
		 (uint8_t)((lba >> 24) & 0xff));
}

static void cmd_out(struct ide_channel* channel, uint8_t cmd) {
	channel->expecting_intr = true;
	outb(reg_cmd(channel), cmd);
}

static void read_from_sector(struct disk* hd, void* buf, uint8_t sec_cnt) {
	uint32_t size_in_byte;
	if (sec_cnt == 0) {
		size_in_byte = 256 * 512;
	} else {
		size_in_byte = sec_cnt * 512;
	}
	insw(reg_data(hd->my_channel), buf, size_in_byte / 2);
}

static void write2sector(struct disk* hd, void* buf, uint8_t sec_cnt) {
	uint32_t size_in_byte;	
	if (sec_cnt == 0) {
		size_in_byte = 256 * 512;
	} else {
		size_in_byte = sec_cnt * 512;
	}
	outsw(reg_data(hd->my_channel), buf, size_in_byte / 2);
}

static bool busy_wait(struct disk* hd) {
	struct ide_channel* channel = hd->my_channel;
	int32_t time_limit = 30 * 1000;

	while ((time_limit -= 10) >= 0) {
		if (!(inb(reg_status(channel)) & BIT_ALT_STAT_BSY)) {
			return true;
			return inb(reg_status(channel) & BIT_ALT_STAT_DRQ);  
		} else {
			mtime_sleep(10);
		}
	}

	return false;
}

void ide_read(struct disk* hd, uint32_t lba, void* buf, uint32_t sec_cnt) {
	ASSERT(lba <= max_lba && sec_cnt > 0);
	
	lock_acquire(&hd->my_channel->lock);
	
	select_disk(hd);
	uint32_t secs_op = 0, secs_done  = 0;

	while(secs_done < sec_cnt) {

		if ((secs_done + 256) <= sec_cnt) {
			secs_op = 256;
		} else { // only left sectors less than 256 to read
			secs_op = sec_cnt - secs_done;
		}
		
		select_sector(hd, lba + secs_done, (uint8_t)(secs_op & 0xff));

		cmd_out(hd->my_channel, CMD_READ_SECTOR);
		sema_down(&hd->my_channel->disk_done);

		/* waked up by disk from interrupt */
		if (!busy_wait(hd)) {
			char err[64];
			sprintf(err, "%s read sector %d failed\n", hd->name, lba + secs_done);
			PANIC(err);
		} 

		read_from_sector(hd, (void*)(buf + secs_done * 512), secs_op);
		secs_done += secs_op;
	}

	lock_release(&hd->my_channel->lock);
}

void ide_write(struct disk* hd, uint32_t lba, void* buf, uint32_t sec_cnt) {
	ASSERT(lba <= max_lba && sec_cnt > 0);
	
	lock_acquire(&hd->my_channel->lock);
	
	select_disk(hd);
	uint32_t secs_op = 0, secs_done  = 0;

	while(secs_done < sec_cnt) {

		if ((secs_done + 256) <= sec_cnt) {
			secs_op = 256;
		} else { // only left sectors less than 256 to read
			secs_op = sec_cnt - secs_done;
		}
		
		select_sector(hd, lba + secs_done, (uint8_t)(secs_op & 0xff));

		cmd_out(hd->my_channel, CMD_WRITE_SECTOR);

		/* waked up by disk from interrupt */
		if (!busy_wait(hd)) {
			char err[64];
			sprintf(err, "%s write sector %d failed\n", hd->name, lba + secs_done);
			PANIC(err);
		} 

		write2sector(hd, (void*)(buf + secs_done * 512), secs_op);
		sema_down(&hd->my_channel->disk_done);
		secs_done += secs_op;
	}

	lock_release(&hd->my_channel->lock);
}

void intr_hd_handler(uint8_t irq_no) {
	ASSERT(irq_no == 0x2e || irq_no == 0x2f);

	uint8_t ch_no = irq_no - 0x2e;
	struct ide_channel* channel = &channels[ch_no];
	ASSERT(channel->irq_no == irq_no);
	
	if (channel->expecting_intr) {
		channel->expecting_intr = false;
		sema_up(&channel->disk_done);
		
		/*
		  read status register to respond intr
		  and continue new operation 
		 */
		inb(reg_status(channel));
	}
}

/* swap len bytes with their own adjacent bytes in dst and save in buf */
static void swap_pairs_bytes(const char* dst, char* buf, uint32_t len) {
	uint8_t idx;
	for (idx = 0; idx < len; idx += 2) {
		buf[idx + 1] = *dst++;
		buf[idx] = *dst++;
	}	

	buf[idx] = '\0';
}

static void identity_disk(struct disk* hd) {
	char id_info[512];	
	select_disk(hd);
	cmd_out(hd->my_channel, CMD_IDENTIFY);
	sema_down(&hd->my_channel->disk_done);

	if (!busy_wait(hd)) {
		char err[64];
		sprintf(err, "%s identity failed\n", hd->name);
	}

	read_from_sector(hd, id_info, 1);

	char buf[64];
	uint8_t sn_start = 10 * 2, sn_len = 20, md_start = 27 * 2, md_len = 40;
	swap_pairs_bytes(id_info + sn_start, buf, sn_len);
	printk("disk %s info :\n    SN:%s\n", hd->name, buf);
	memset(buf, 0, sizeof(buf));
	swap_pairs_bytes(id_info + md_start, buf, md_len);
	printk("    MODULE:%s\n", buf);
	uint32_t sectors = *((uint32_t*)(id_info + 120));
	printk("    SECTORS:%d\n", sectors);
	printk("    CAPACITY:%dMB\n", sectors * 512 / 1024 / 1024);
}

static void partition_scan(struct disk* hd, uint32_t ext_lba) {
	struct boot_sector* bs = sys_malloc(sizeof(struct boot_sector));
	ide_read(hd, ext_lba, bs, 1);
	
	uint8_t part_idx = 0;
	struct partition_table_entry* p = bs->partition_table;
	
	while (part_idx++ < 4) {
		if (p->fs_type == 0x5) {
			if (ext_lba_base != 0) {
				partition_scan(hd, p->start_lba + ext_lba_base);
			} else { // first time get extend partition in MBR
				ext_lba_base = p->start_lba;
				partition_scan(hd, p->start_lba);
			} 
		} else if (p->fs_type != 0) {
			if (ext_lba == 0) {
				ASSERT(p->fs_type == 0x83);
				hd->prim_parts[p_no].start_lba = ext_lba + p->start_lba;
				hd->prim_parts[p_no].sec_cnt = p->sec_cnt;
				printk("pp:%d\n", p->sec_cnt);
				hd->prim_parts[p_no].my_disk = hd;
				list_append(&partition_list, &hd->prim_parts[p_no].part_tag);
				sprintf(hd->prim_parts[p_no].name, "%s%d", hd->name, p_no + 1);
				p_no++;
				ASSERT(p_no < 4);
			} else if (l_no < 8) {
				ASSERT(p->fs_type == 0x66);
				hd->logic_parts[l_no].start_lba = ext_lba + p->start_lba;
				hd->logic_parts[l_no].sec_cnt = p->sec_cnt;
				hd->logic_parts[l_no].my_disk = hd;
				list_append(&partition_list, &hd->logic_parts[l_no].part_tag);
				sprintf(hd->logic_parts[l_no].name, "%s%d", hd->name, l_no + 5);
				l_no++;
			}
		}
		p++;
	}
	sys_free(bs);
}

static bool partition_info(struct list_elem* pelem, int arg UNUSED) {
	struct partition* part = elem2entry(struct partition, part_tag, pelem);

	printk("%s start_lba:0x%x, sec_cnt:0x%x\n", part->name, part->start_lba, part->sec_cnt);
	return false; // to keep list_traversal run
}
