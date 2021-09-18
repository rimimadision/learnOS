#include "buildin_cmd.h"
#include "dir.h"
#include "fs.h"

static void wash_path(char* old_abs_path, char* new_abs_path) {
	ASSERT(old_abs_path[0] == '/');
	char name[MAX_FILE_NAME_LEN] = {0};
	char* sub_path = old_abs_path;
	sub_path = path_parse(sub_path, name);
	if (name[0] == 0) {
		new_abs_path[0] = '/';
		new_abs_path[1] = 0;	
		return;
	}
	new_abs_path[0] = 0;
	strcat(new_abs_path, "/");
	while(name[0]) {
		if ()
	}
}
