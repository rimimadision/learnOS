#include "stdio.h"

void panic_spin(char* filename, int line, const char* func, const char* condition)
{
	printf("\n\n error_occur \n\n");
	printf("\nfilename:%s", filename); 
	printf("\nline:0x%x", line); 
	printf("\nfunction:%s", func); 
	printf("\ncondition:%s\n", condition);
	while(1);
}
