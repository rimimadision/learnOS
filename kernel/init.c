#include "init.h"
#include "print.h"
#include "interrupt.h"
#include "../device/timer.h"
#include "memory.h"

void init_all()
{
	put_str("init_all\n");
	idt_init();
	timer_init();
	mem_init();
	put_str("init_all done\n");
} 
