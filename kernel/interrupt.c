#include"interrupt.h"
#include"stdint.h"
#include"global.h"
#include"io.h"

#define IDT_DESC_CNT 0X21
#define PIC_M_CTRL 0X20
#define PIC_M_DATA 0X21
#define PIC_S_CTRL 0Xa0
#define PIC_S_DATA 0Xa1

/* interrupt gate descriptor structure*/
struct gate_desc {
	uint16_t func_offset_low_word;	
	uint16_t selector;
	uint8_t dcount; // fixed value = 0 for IDT
	uint8_t attribute;
	uint16_t func_offset_high_word;
};

void idt_init();

static void pic_init();
static void make_idt_desc(struct gate_desc* p_gdesc, uint8_t attr, intr_handler function);
static struct gate_desc idt[IDT_DESC_CNT];// IDT structure

extern intr_handler intr_entry_table[IDT_DESC_CNT];

/* init pic */
static void pic_init()
{
	outb(PIC_M_CTRL, 0X11); // ICW1
	outb(PIC_M_DATA, 0X20); // ICW2, start with 0x20 interrupt_code
	outb(PIC_M_DATA, 0X04); // ICW3, IR2 link slave
	outb(PIC_M_DATA, 0X01); // ICW4, normal EOI
	
	outb(PIC_S_CTRL, 0X11); // ICW1	
	outb(PIC_S_DATA, 0X28); // ICW2, start with 0x28 interrupt_code
	outb(PIC_S_DATA, 0X02); // ICW3, IR2 link master
	outb(PIC_S_DATA, 0X01); // ICW4, normal EOI

	/* mask IR1 - IR15, only use IR0 */
	outb(PIC_M_DATA, 0Xfe);
	outb(PIC_S_DATA, 0Xff);
	
	put_str("pic_init donn\n");
}


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
	int i = 0;
	for(i = 0; i < IDT_DESC_CNT; i++)
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
	//uint64_t idt_operand = ((sizeof(idt) - 1) | ((uint64_t)((uint32_t)idt << 16)));
	uint64_t idt_operand = ((sizeof(idt) - 1) | ((uint64_t)((uint32_t)idt) << 16));
	asm volatile ("lidt %0" :: "m"(idt_operand));
	put_str("idt_init done\n");
}
