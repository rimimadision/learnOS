#include "bitmap.h"

/* set bits 0 */
inline void bitmap_init(struct bitmap* btmp)
{
	memset(btmp->btmp_addr, 0, btmp->btmp_bytes_len);
}

/* test bit on bit_idx 0 or 1*/
inline int bitmap_test_bit(struct bitmap* btmp, uint32_t bit_idx)
{
	uint32_t byte_idx = bit_idx / 8;
	uint32_t bit_odd = bit_idx % 8;
	return (btmp->btmp_addr[byte_idx] & (BITMAP_MASK << bit_odd)) ? 1 : 0;
}

/* find continous 'cnt' empty pages */
int bitmap_scan(struct bitmap* btmp, uint32_t cnt)
{	
	uint32_t idx_byte = 0;
	while((0xff == btmp->btmp_addr[idx_byte]) && (idx_byte < btmp->btmp_bytes_len))
	{
		idx_byte++;
	}	
	ASSERT(idx_byte < btmp->btmp_bytes_len);
	if(idx_byte == btmp->btmp_bytes_len)
	{
		// No empty page in bitmap
		return -1;
	}

	/* have found the empty page in bitmap */
	uint32_t idx_bit = 0;
	while((uint8_t)(BITMAP_MASK << idx_bit) & btmp->btmp_addr[idx_byte])
	{
		idx_bit++;
	}
	
	uint32_t bit_idx_start = idx_byte * 8 + idx_bit;
	if(cnt == 1)
	{
		return bit_idx_start;
	}
	
	/* cnt > 1 */
	uint32_t next_bit = bit_idx_start + 1;
	uint32_t cur_count = 1;
	uint32_t total_bits = btmp->btmp_bytes_len * 8;

	while(next_bit < total_bits)
	{
		if((bitmap_test_bit(btmp, next_bit)) == 0)
		{
			cur_count++;
		}else
		{
			cur_count = 0;
		}
		
		if(cur_count == cnt)
		{
			return (next_bit - cnt + 1);
		}
		next_bit ++;
	}

	return -1;
}
void bitmap_set(struct bitmap* btmp, uint32_t bit_idx, int8_t value)
{
	ASSERT(value == 0 || value == 1)
	uint32_t byte_idx = bit_idx / 8;
	uint32_t bit_odd  = bit_idx % 8;
	
	if(value == 1)
	{
		btmp->btmp_addr[byte_idx] |= (BITMAP_MASK << bit_odd);
	}else
	{
		btmp->btmp_addr[byte_idx] &= ~(BITMAP_MASK << bit_odd);
	}
}
