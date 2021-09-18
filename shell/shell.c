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

static char cmd_line[cmd_len] = {0};
char cwd_cache[64] = {0};
char* argv[MAX_ARG_NR];
char* final_path[cmd_len];
int32_t argc = -1;

static int32_t cmd_parse(char* cmd_str, char** argv, char token);

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
				//putchar('\n');
				return;
			case '\b':
				if (buf[0] != '\b') {
					pos--;
					//putchar('\b');
				}
				break;
			case 'l' - 'a':
				*pos = 0;
				clear();
				print_prompt();
				printf("%s", buf);
				break;
			case 'u' - 'a':
				while (buf != pos) {
					putchar('\b');
					*(pos--) = 0;
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
		memset(final_path, 0, cmd_len);
		memset(cmd_line, 0, cmd_len);
		readline(cmd_line, cmd_len);
		if (cmd_line[0] == 0) {
			continue;
		}

		argc = -1;
		argc = cmd_parse(cmd_line, argv, ' ');
		if (argc == -1) {
			printf("num of arguments exceed %d\n", MAX_ARG_NR);
			continue;
		}
		
		int32_t arg_idx = 0;
		while (arg_idx < argc) {
			printf("%s/", argv[arg_idx]);
			arg_idx++;
		}
		printf("\n");
	}	
	PANIC("my_shell: should not be here");
}

static int32_t cmd_parse(char* cmd_str, char** argv, char token) {
	ASSERT(cmd_str != NULL);
	int32_t arg_idx = 0;
	while (arg_idx < MAX_ARG_NR) {
		argv[arg_idx] = NULL;
		arg_idx++;
	}

	char* next = cmd_str;
	int32_t argc = 0;

	while (*next) {
		while (*next == token) {
			next++;
		}
		if (*next == 0) {
			break;
		}
		argv[argc] = next;
		while (*next && *next != token) {
			next++;
		}

		if (*next) {
			*next++ = 0;
		}

		if (argc > MAX_ARG_NR) {
			return -1;
		}
		argc++;
	}
	
	return argc;
}	
