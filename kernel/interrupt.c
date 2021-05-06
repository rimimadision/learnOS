#include"interrupt.h"
#include"stdint.h"
#include"global.h"
#include"io.h"

#define IDT_DESC_CNT 0X21

struct gate_desc {
	uint16_t func_offset_low_word;	
	uint16_t selector;
	uint8_t dcount; // fixed value = 0 for IDT
	uint8_t attribute;
	uint16_t func_offset_high_word;
};

static void make_idt_desc(struct gate_desc* p_gdesc, uint8_t attr, intr_handler function);
static struct gate_desc idt[IDT_DESC_CNT];


