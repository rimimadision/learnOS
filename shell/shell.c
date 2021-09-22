#include "shell.h"
#include "list.h"
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
#include "buildin_cmd.h"

static char cmd_line[cmd_len] = {0};
char final_path[MAX_PATH_LEN] = {0};
char cwd_cache[MAX_PATH_LEN] = {0};
char* argv[MAX_ARG_NR];
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
				return;
			case '\b':
				if (buf[0] != '\b') {
					pos--;
				}
				break;
			default: 
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
		memset(final_path, 0, MAX_PATH_LEN);
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
		
		if (!strcmp("pwd", argv[0])) {
			buildin_pwd(argc, argv);
		} else if (!strcmp("ps", argv[0])) {
			buildin_ps(argc, argv);
		} else if (!strcmp("clear", argv[0])) {
			buildin_clear(argc, argv);
		} else if (!strcmp("cd", argv[0])) {
			if (buildin_cd(argc, argv) != NULL) {
				memset(cwd_cache, 0, MAX_PATH_LEN);
				strcpy(cwd_cache, final_path);
			}
		} else if (!strcmp("ls", argv[0])) {
			buildin_ls(argc, argv);
		} else if (!strcmp("mkdir", argv[0])){
			buildin_mkdir(argc, argv);
		} else if (!strcmp("rmdir", argv[0])){
			buildin_rmdir(argc, argv);
		} else if (!strcmp("rm", argv[0])){
			buildin_rm(argc, argv);
		} else {
			int32_t pid = fork();
			if (pid != 0) { // parent process
				while(1);
			} else {
				make_clear_abs_path(argv[0], final_path);
				argv[0] = final_path;
				struct stat file_stat;
				memset(&file_stat, 0, sizeof(struct stat));
				if (stat(argv[0], &file_stat) == -1) {
					printf("my_shell:cannot access %s: No such file or directory\n", argv[0]);
				} else {
					execv((const char*)argv[0], (const char**)argv);
				}
				while(1);
			}
		}
		memset(argv, 0, MAX_ARG_NR);	
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
