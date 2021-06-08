#ifndef __LIB_KERNEL_BITMAP_H
#define __LIB_KERNEL_BITMAP_H

#include "global.h"
#include "stdint.h"
#include "string.h"
#include "print.h"
#include "interrupt.h"
#include "debug.h"

#define BITMAP_MASK 1

struct bitmap
{
	uint32_t btmp_bytes_len; // page_cnt = btmp_bytes_len * 8
	uint8_t* btmp_addr;
};

void bitmap_init(struct bitmap* btmp);
bool bitmap_test_bit(struct bitmap* btmp, uint32_t bit_idx);
int bitmap_scan(struct bitmap* btmp, uint32_t cnt);
void bitmap_set(struct bitmap* btmp, uint32_t bit_idx, int8_t value);

#endif //__LIB_KERNEL_BITMAP_H

