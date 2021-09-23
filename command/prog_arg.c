#include "stdio.h"
#include "syscall.h"
#include "string.h"

int main(int argc, char* argv[]) {
	int arg_idx = 0;
	while (arg_idx < argc) {
		printf("argv[%d] is %s\n", arg_idx, argv[arg_idx]);
		arg_idx++;
	}	

	int pid = fork();
	if (pid) { // parent
		long long delay = 90000000;
		while(delay--);
		printf("\nfather, pid:%d\n", getpid());
		ps();
	} else { // child
		printf("\nchild, pid:%d\n", getpid());
		if (argv[1][0] != '/') {
			char abs_path[512] = {0};
			getcwd(abs_path, 512);
			strcat(abs_path, "/");
			strcat(abs_path, argv[1]);
			execv((const char*)abs_path, (const char**)argv);
		} else {
			execv((const char*)argv[1], (const char**)argv);
		}	
	}

	while(1);
	return 0;
}
