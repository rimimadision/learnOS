#ifndef __SHELL_SHELL_H
#define __SHELL_SHELL_H

#include "fs.h"

#define MAX_ARG_NR 5
#define cmd_len 64

extern char* argv[MAX_ARG_NR];
extern char final_path[MAX_PATH_LEN];

void print_prompt(void);
void my_shell(void);

#endif // __SHELL_SHELL_H
