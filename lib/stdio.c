#include "stdio.h"
#include "global.h"
#include "stdint.h"
#include "syscall.h"
#include "string.h"

static void itoa(uint32_t value, char** buf_ptr_addr, uint8_t base) {
	int m = value % base;
	int i = value / base;
	
	if(i) {
		itoa(i, buf_ptr_addr, base);
	}
	
	if(m < 10) { // 0 ~ 9
		*((*buf_ptr_addr)++) = m + '0';
	} else {
		*((*buf_ptr_addr)++) = m - 10 + 'A';
	}
}

uint32_t vsprintf(char* str, const char* format, va_list ap) {
	char* buf_str = str;
	const char* index_ptr = format;
	char index_char = *(index_ptr);
	int32_t arg_int;	
	char* arg_str;

	while(index_char) {
		if(index_char != '%') {
			*(buf_str++) = index_char;
			index_char = *(++index_ptr);
			continue;
		}
		index_char = *(++index_ptr);
		switch(index_char) {
			case 's':
				arg_str = va_arg(ap, char*);
				strcpy(buf_str, arg_str);
				buf_str += strlen(arg_str);
				index_char = *(++index_ptr);
				break;
			case 'c':
				*(buf_str++) = va_arg(ap, char);
				index_char = *(++index_ptr);
				break;
			case 'd':
				arg_int = va_arg(ap, int);
				if(arg_int < 0) {
					arg_int = -arg_int;
					*(buf_str++) = '-';
				}	
				itoa(arg_int, &buf_str, 10);
				index_char = *(++index_ptr);
				break;
			case 'x':
				arg_int = va_arg(ap, int);
				itoa(arg_int, &buf_str, 16);	
				index_char = *(++index_ptr);
				break;
		}
	}
	*buf_str = '\0';

	return strlen(str);
}

uint32_t printf(const char* format, ...) {
	va_list args;
	va_start(args, format);
	char buf[1024] = {0};
	vsprintf(buf, format, args);
	va_end(args);
	return write(1, buf, strlen(buf));
}

uint32_t sprintf(char* buf, const char* format, ...) {
	va_list args;
	uint32_t retval;
	va_start(args, format);
	retval = vsprintf(buf, format, args);
	va_end(args);
	return retval;
}
