#include "timer.h"
#include "io.h"
#include "print.h"
#include "thread.h"
#include "interrupt.h"
#include "debug.h"
#include "stdint.h"

#define IRQ0_FREQUENCY 100
#define INPUT_FREQUENCY 1193180
#define COUNTER0_VALUE (INPUT_FREQUENCY / IRQ0_FREQUENCY)
#define COUNTER0_PORT 0x40
#define COUNTER0_NO 0
#define COUNTER_MODE 2
#define READ_WRITE_LATCH 3
#define PIT_CONTROL_PORT 0x43

uint32_t total_ticks;

static void frequency_set(uint8_t counter_port, uint8_t counter_no, uint8_t rwl, uint8_t counter_mode, uint16_t counter_value);
static void intr_timer_handler(void);

void timer_init(void);

static void frequency_set(uint8_t counter_port,
						  uint8_t counter_no,
						  uint8_t rwl,
						  uint8_t counter_mode,
						  uint16_t counter_value)
{
	// set counter
	outb(PIT_CONTROL_PORT, (uint8_t)(counter_no << 6 | rwl << 4 | counter_mode << 1));
	
	// set initial value of counter 
	outb(counter_port, (uint8_t)counter_value);
	outb(counter_port, (uint8_t)(counter_value >> 8));
}

void timer_init(void)
{
	put_str("timer_init start\n");
	frequency_set(COUNTER0_PORT, COUNTER0_NO, READ_WRITE_LATCH, COUNTER_MODE, COUNTER0_VALUE);
	register_handler(0x20, intr_timer_handler);
	put_str("timer_iniit done\n");

}

static void intr_timer_handler(void)
{
	struct task_struct* cur_thread = get_cur_thread_pcb();
	ASSERT(cur_thread->stack_magic == 0x19700505);	
	put_str("????\n");
	cur_thread->elapsed_ticks++;
	total_ticks++;
	if(cur_thread->ticks == 0)
	{
		schedule();
	}else
	{
		cur_thread->ticks--;
	}
}
