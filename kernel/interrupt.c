#include "interrupt.h"
#include "stdint.h"
#include "global.h"
#include "io.h"
#include "print.h"

#define IDT_DESC_CNT 0X21
#define PIC_M_CTRL 0X20
#define PIC_M_DATA 0X21
#define PIC_S_CTRL 0Xa0
#define PIC_S_DATA 0Xa1

#define EFLAGS_IF 0x00000200
#define GET_EFLAGS(EFLAG_VAR) asm volatile("pushfl; popl %0" : "=g"(EFLAG_VAR))

/* interrupt gate descriptor structure*/
struct gate_desc {
	uint16_t func_offset_low_word;	
	uint16_t selector;
	uint8_t dcount; // fixed value = 0 for IDT
	uint8_t attribute;
	uint16_t func_offset_high_word;
};

void idt_init(void);

static void pic_init(void);
static void make_idt_desc(struct gate_desc* p_gdesc, uint8_t attr, intr_handler function);
static void general_intr_handler(uint8_t vec_nr);
static void exception_init(void);
static void set_cursor(uint32_t pos);

enum intr_status intr_enable(void);
enum intr_status intr_disable(void);
enum intr_status intr_set_status(enum intr_status status);
enum intr_status intr_get_status(void);

char* intr_name[IDT_DESC_CNT]; // Hold the name of exception
static struct gate_desc idt[IDT_DESC_CNT]; // IDT structure
intr_handler idt_table[IDT_DESC_CNT]; // Entry of interrupt handler function 
extern intr_handler intr_entry_table[IDT_DESC_CNT]; // Entry of interrupt function 

/* init pic */
static void pic_init(void)
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
	
	put_str("pic_init done\n");
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

/* general interrupt handler, normally for exception */
static void general_intr_handler(uint8_t vec_nr)
{
	if(vec_nr == 0x27 || vec_nr == 0x2f) // IRQ7 and IRQ15 may generate spurious interrupt, 
	{									 // can't be masked off, ignore it
		return;
	}
	put_str("int vector : 0x");
	put_int(vec_nr);
	put_char('\n');
	// interrupt handler to be extended here
}
/* register general exception handler and name 0-19 exceptions */
static void exception_init(void)
{
	int i = 0;
	for(i = 0; i < IDT_DESC_CNT; i ++)
	{
		idt_table[i] = general_intr_handler;
		intr_name[i] = "unknown";
	}
	
	/* name exception from 0 to 19*/
	intr_name[0] = "#DE Divide Error";
	intr_name[1] = "#DB Debug Exception";
	intr_name[2] = "NMI Interrupt";
	intr_name[3] = "#BP Breakpoint Exception";
	intr_name[4] = "#OF Overflow Exception";
	intr_name[5] = "#BR BOUND Range Exceeded Exception";
	intr_name[6] = "#UD Invalid Opcode Exception";
	intr_name[7] = "#NM Device Not Available Exception";
	intr_name[8] = "#DF Double Fault Exception";
	intr_name[9] = "#MF Coprocessor Segment Overrun";
	intr_name[10] = "#TS Invalid TSS Exception";
	intr_name[11] = "#NP Segment Not Present";
	intr_name[12] = "#SS Stack Fault Exception";
	intr_name[13] = "#GP General Protection Exception";
	intr_name[14] = "#PF Page-Fault Exception";
	//intr_name[15] reserved by INTEL
	intr_name[16] = "#MF x87 FPU Floating-Point Error";
	intr_name[17] = "#AC Alignment Check Exception";
	intr_name[18] = "#MC Machine-Check Exception";
	intr_name[19] = "#XF SIMD Floating-Point Exception";

}
void idt_init(void)
{
	put_str("idt_init start\n");
	idt_desc_init();
	exception_init();
	pic_init();// init 8259A
	
	/* load idt */
	uint64_t idt_operand = ((sizeof(idt) - 1) | ((uint64_t)((uint32_t)idt) << 16));
	asm volatile ("lidt %0" :: "m"(idt_operand));
	put_str("idt_init done\n");
}

/* set IF 1 to open interrupt, return IF before */
enum intr_status intr_enable(void)
{
	enum intr_status old_status = intr_get_status();
	if(old_status == INTR_OFF)
	{
		asm volatile("sti");
	}
	return old_status;
}

/* set IF 0 to close interrupt, return IF before */
enum intr_status intr_disable(void)
{
	enum intr_status old_status = intr_get_status();
	if(old_status == INTR_ON)
	{
		asm volatile("cli" : : : "memory");
	}
	return old_status;
}	

/* set IF, return IF before */
enum intr_status intr_set_status(enum intr_status status)
{
	return (status & INTR_ON) ? intr_enable() : intr_disable();
}

/* get current IF */
enum intr_status intr_get_status(void)
{
	uint32_t eflags = 0;
	GET_EFLAGS(eflags);
	return (eflags & eflags) ? INTR_ON : INTR_OFF;
}

/* set cursor to position 'pos' */
static void set_cursor(uint32_t pos)
{
	asm volatile("movl 0x03d4, dx"


				 : : "b"(pos));

}
