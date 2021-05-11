#include"interrupt.h"
#include"stdint.h"
#include"global.h"
#include"io.h"

#define IDT_DESC_CNT 0X21

/* interrupt gate descriptor structure*/
struct gate_desc {
	uint16_t func_offset_low_word;	
	uint16_t selector;
	uint8_t dcount; // fixed value = 0 for IDT
	uint8_t attribute;
	uint16_t func_offset_high_word;
};

void idt_init();

static void make_idt_desc(struct gate_desc* p_gdesc, uint8_t attr, intr_handler function);
static struct gate_desc idt[IDT_DESC_CNT];// IDT structure

extern intr_handler intr_entry_table[IDT_DESC_CNT];

/* create interrupt descriptor */
static void make_idt_desc(struct gate_desc* p_gdesc, uint8_t attr, intr_handler function)
{
	p_gdesc->func_offset_low_word = (uint16_t)((uint32_t)function & 0x0000ffff);
	p_gdesc->selector = SELECTOR_K_CODE;
	p_gdesc->dcount = 0;
	p_gdesc->attribute = attr;
	p_gdesc->func_offset_high_word = (uint16_t)(((uint32_t)function & 0xffff0000) >> 16);
}

/* init interrupt descriptor */
static void idt_desc_init(void)
{
	for(int i = 0; i < IDT_DESC_CNT; i++)
	{
		make_idt_desc(&idt[i], IDT_DESC_ATTR_DPL0, intr_entry_table[i]);
	}
	put_str(" idt_desc_init done\n");
}

void idt_init()
{
	put_str("idt_init start\n");
	idt_desc_init();
	pic_init();// init 8259A
	
	/* load idt */
	uint64_t idt_operand = ((sizeof(idt) - 1) | ((uint64_t)idt << 16));
	asm volatile ("lidt %0" :: "m"(idt_operand));
	put_str("idt_init done\n");
}
