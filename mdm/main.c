#include "mdm.h"

int is_launch_x = 1;

int main(int argc, char** argv){
	int i, st;
	
	for(i = 1; i < argc; i++){
		if(argv[i][0] == '-'){
			if(strcmp(argv[i], "-X") == 0 || strcmp(argv[i], "--no-launch-x") == 0){
				is_launch_x = 0;
			}else{
				fprintf(stderr, "%s: bad option: %s\n", argv[0], argv[i]);
				return 1;
			}
		}
	}

	if(access(CONFDIR "/mdm", F_OK) != 0) mkdir(CONFDIR "/mdm", 0755);
	if(access(CONFDIR "/mdm/mdmrc", F_OK) != 0) MDEFileCopy(DATADIR "/examples/mdm/mdmrc", CONFDIR "/mdm/mdmrc");

	parse_config();

	if(is_launch_x){
		if((st = launch_x()) != 0) return st;
	}

	init_x();

	login_window();
}
