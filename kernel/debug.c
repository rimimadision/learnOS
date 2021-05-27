#include "debug.h"
#include "print.h"
#include "interrupt.h"

void panic_spin(char* filename, int line, const char* func, const char* condition)
{
	intr_disable();

	put_str("\n\n error_occur \n\n");
	put_str("\nfilename:"); put_str(filename);
	put_str("\nline:0x"); put_int(line);
	put_str("\nfunction:"); put_str(func);
	put_str("\ncondition:"); put_str(condition);
	put_str("\n");
	while(1);
}
