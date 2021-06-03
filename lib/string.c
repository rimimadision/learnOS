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
	while(size--)
	{
		if(*a != *b)
		{
			return (*a > *b) ? 1 : -1;
		}
		a++;
		b++;
	}
	return 0;
}   
 
/* copy str from src_ to dst_, return dst_ */
char* strcpy(char* dst_, const char* src_)
{
	ASSERT(dst_ != NULL && src_ != NULL);
	char* tmp = dst_;
	while(*dst_++ = *src_++);
	return tmp;
}

/* return string length */
uint32_t strlen(const char* str)
{
	ASSERT(str != NULL);
	const char* p = str;
	while(*p++);
	return (p - str -1);
}

/* compare a and b string
   if equal, return 0,
   if a > b, return 1,
   if a < b, return -1*/
int8_t strcmp(const char* aStr, const char* bStr)
{
	ASSERT(aStr != NULL && bStr != NULL);
	while(*aStr != '\0' && *aStr == *bStr)
	{
		aStr++;
		bStr++;
	}
	
	return (*aStr < *bStr) ? -1 : (*aStr > *bStr);
}

/* find the 'ch' from front, return address of 'ch' */
char* strchr(const char* str, const char ch)
{
	ASSERT(str != NULL && ch != '\0');
	while(*str != '\0')
	{
		if(*str == ch)
		{
			return (char*)str;
		}
		str++;
	}	
	
	return NULL:
}

/* find the 'ch' from back, return address of 'ch' */
char* strrchr(const char* str, const char ch)
{
	ASSERT(str != NULL && ch != '\0');
	char* last_char = NULL;
	while(*str != '\0')
	{
		if(*str == ch)
		{
			last_char = str;
		}
		str++;
	}
	return (char*)last_char;
}

/* cat src_ behind dst_, return dst_ */
char* strcat(char* dst_, const char* src_)
{
	ASSERT(dst_ != NULL && src_ != NULL);
	char* p = dst_;
	while(*p++);
	p--;
	while(*p++ = *src_++);
	
	return dst_
}
/* return times ch appears */
uint32_t strchrs(const char* str, uint8_t ch)
{
	ASSERT(str != NULL);
	uint32_t times = 0;
	const char* p = str;
	while(*p != '\0')
	{
		if(*p == ch)
		{
			times++;
		}
		p++;
	}
	
	return times;
}
