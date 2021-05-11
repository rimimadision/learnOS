#ifndef __LIB_IO_H
#define __LIB_IO_H

#include "stdint.h"

/* write 1 byte to port */
static inline void outb(unint16_t port, uint8_t data)
{
	asm volatile("outb %b0, %w1" : "a"(data) : "d"(port))// b0-al w1-dx
} 

/* write word_cnt bytes into port */
static inline void outsw() 
