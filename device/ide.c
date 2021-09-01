#include "ide.h"
#include "global.h"
#include "stdio.h"

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
#define BIT_DEV_MBS (1 << 7 + 1 << 5)
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
struct ide_channel channels[CHANNEL_CNT];

void ide_init() {
	printk("ide_init start\n");
	
	uint8_t hd_cnt = *((uint8_t*)(0x475));
	channel_cnt = DIV_ROUND_UP(hd_cnt, 2); // I don't know how to get channel used by computer
	struct ide_channel* channel;
	uint8_t channel_no = 0;
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
		channel_no ++;
	}

	printk("ide_init done\n");
}

static void select_disk(struct disk* hd) {
	
}
