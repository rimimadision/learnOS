#ifndef __LIB_STRING_H
#define __LIB_STRING_H

/* function about manipulating memory */
void memset(void* dst_, uint8_t value, uint32_t size);
void memcpy(void* dst_, const void* src_, uint32_t size);
int8_t memcmp(const void* dst_, const void* src_, uint32_t size);

/* function about manipulating string */
char* strcpy(char* dst_, const char* src_);
uint32_t strlen(const char* str);
int8_t strcmp(const char* aStr, const char* bStr);
char* strchr(const char* str, const char ch);
char* strrchr(const char* str, const char ch);
char* strcat(char* dst_, const char* src_);
uint32_t strchrs(const char* str, uint8_t ch);
#endif // __LIB_STRING_H
