#include "syscall.h"

#define _syscall0(NUMBER) ({ \
		int retval;\
		asm volatile (\
			"int $0x80"\
			: "=a" (retval)\
			: "a" (NUMBER)\
			: "memory"\
		);\
		retval;\
})

#define _syscall3(NUMBER, ARG1, ARG2, ARG3) ({ \
		int retval;\
		asm volatile (\
			"int $0x80"\
			: "=a" (retval)\
			: "a" (NUMBER), "b" (ARG1), "c" (ARG2), "d" (ARG3)\
			: "memory"\
		);\
		retval;\
})

uint32_t getpid() {
	return _syscall0(SYS_GETPID);
}
