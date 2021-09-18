#ifndef __SHELL_SHELL_H
#define __SHELL_SHELL_H

#define MAX_ARG_NR 5
#define cmd_len 64

extern char* argv[MAX_ARG_NR];
extern char final_path[cmd_len];

void print_prompt(void);
void my_shell(void);

#endif // __SHELL_SHELL_H
