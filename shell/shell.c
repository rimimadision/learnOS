#include "shell.h"
#include "stdio.h"
#include "debug.h"
#include "file.h"
#include "string.h"
#include "stdint.h"
#include "syscall.h"
#include "stdio.h"
#include "fs.h"
#include "ioqueue.h"
#include "keyboard.h"

#define cmd_len 128
#define MAX_ARG_NR 16

static char cmd_line[cmd_len] = {0};
char cwd_cache[64] = {0};

void print_prompt(void) {
	printf("[rimi@localhost %s]$ ", cwd_cache);
}

static void readline(char* buf, int32_t count) {
	ASSERT(buf != NULL && count > 0);
	char* pos = buf;
	ioqueue_init(&kbd_buf);
	while (read(stdin_no, pos, 1) != -1 && (pos - buf) < count) {
		switch (*pos) {
			case '\n':
			case '\r':
				*pos = 0;
				printf("%s\n", cmd_line);
				//putchar('\n');
				return;
			case '\b':
				if (buf[0] != '\b') {
					pos--;
					//putchar('\b');
				}
				break;
			default: 
				//putchar(*pos);
				pos++;
		}
	}
	
	printf("readline: can't find enter_key in the cmd_line, max num of char is 128\n");
}

void my_shell(void) {
	cwd_cache[0] = '/';
	while(1) {
		print_prompt();
		memset(cmd_line, 0, cmd_len);
		readline(cmd_line, cmd_len);
		if (cmd_line[0] == 0) {
			continue;
		}
	}	
	PANIC("my_shell: should not be here");
}

