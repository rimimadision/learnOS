#include "string.h"
#include "global.h"
#include "string.h"

/* set total "size" bytes "value" from "dst_"*/
void memset(void* dst_, uint8_t value, uint32_t size)
{
	ASSERT(dst_ != NULL);
	uint8_t dst_bytes = (uint8_t)dst_;
 	while(size--)
 	{
		*dst_bytes++ = value;
	}
}
 
/* copy total "size" bytes from dst_ to src_ */
void memcpy(void* dst_, const void* src_, uint32_t size)
{
	ASSERT(dst_ != NULL && src_ != NULL);
	uint8_t dst_bytes = (uint8_t)dst_;
	const uint8_t src_bytes = (const uint8_t)src_;

	while(size--)
	{   
	   *dst_bytes++ = *src_bytes;
	}
}   
 
/*compare total "size" bytes between  a_ and b_, 
  if equal, return 0,
  if a_ > b_, return 1,
  if a_ < b_, return -1*/
int8_t memcmp(const void* a_, const void* b_, uint32_t size)
{   
	ASSERT(a_ != NULL && b_ != NULL);
	const char* a_bytes = a_;
  	const char* b_bytes = b_;
}   
 
void strcpy(char* dst_, const char* src_);
void strlen(const char* str); 
int8_t strcmp(const char* aStr, const char* bStr);
char* strchr(const char* str, const char ch);
char* strrchr(const char* str, const char ch);
char* strcat(char* dst_, const char* src_);
uint32_t strchrs(const char* str, uint8_t ch);
