#ifndef __LIB_IO_H
#define __LIB_IO_H

#include "stdint.h"

/* write 1 byte to port */
static inline void outb(uint16_t port, uint8_t data)
{
	asm volatile("outb %b0, %w1" : : "a"(data), "Nd"(port));// b0-al w1-dx
} 

/* write word_cnt bytes into port */
static inline void outsw(uint16_t port, const void* addr, uint32_t word_cnt)
{
	asm volatile("cld; rep outsw" : "+S"(addr), "+c"(word_cnt) : "d"(port));
} 

/* read 1 byte from port */
static inline uint8_t inb(uint16_t port)
{
	uint8_t data;
	asm volatile("inb %w1, %b0" : "=a"(data) : "Nd"(port));
	return data;
}

/* read word_cnt bytes from port */
static inline void insw(uint16_t port, void* addr, uint32_t word_cnt)
{
	asm volatile("cld; rep insw" : "+D"(addr), "+c"(word_cnt) : "d"(port) : "memory");
}

#endif
