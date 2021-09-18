#ifndef __SHELL_SHELL_H
#define __SHELL_SHELL_H

#define MAX_ARG_NR 16
extern char* argv[MAX_ARG_NR];

void print_prompt(void);
void my_shell(void);

#endif // __SHELL_SHELL_H
