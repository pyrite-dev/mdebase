#include "mdm.h"

int is_launch_x = 1;
int is_daemon = 1;

gid_t gid;
uid_t uid;
char* run;

int main(int argc, char** argv) {
	int   i, st;
	char* backup = NULL;
	pid_t pid;

	for(i = 1; i < argc; i++) {
		if(argv[i][0] == '-') {
			if(strcmp(argv[i], "-X") == 0 || strcmp(argv[i], "--no-launch-x") == 0) {
				is_launch_x = 0;
			}else if(strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--no-daemon") == 0) {
				is_daemon = 0;
			} else {
				fprintf(stderr, "%s: bad option: %s\n", argv[0], argv[i]);
				return 1;
			}
		}
	}

	if(getuid() != 0) {
		fprintf(stderr, "MDM has to be ran as root\n");
		return 1;
	}

	if(is_daemon && (pid = fork()) != 0){
		FILE* f = fopen("/var/run/mdm.pid", "w");
		fprintf(f, "%d\n", pid);
		fclose(f);
		return 0;
	}

	if(access(CONFDIR, F_OK) != 0) mkdir(CONFDIR, 0755);
	if(access(CONFDIR "/mdm", F_OK) != 0) mkdir(CONFDIR "/mdm", 0755);
	if(access(CONFDIR "/mdm/mdmrc", F_OK) != 0) MDEFileCopy(DATADIR "/examples/mdm/mdmrc", CONFDIR "/mdm/mdmrc");

	parse_config();

	if(getenv("DISPLAY") != NULL) backup = MDEStringDuplicate(getenv("DISPLAY"));
	do {
		setenv("DISPLAY", backup == NULL ? "" : backup, 1);
		if(is_launch_x && (st = launch_x()) != 0) return st;

		if((st = init_x()) != 0) return st;

		login_window();

		if(run == NULL) continue;

		if((pid = fork()) == 0) {
			struct passwd* pwd = getpwuid(uid);

			setgid(gid);
			setuid(uid);
			chdir(pwd->pw_dir);
			setenv("USER", pwd->pw_name, 1);
			setenv("SHELL", pwd->pw_shell, 1);
			setenv("HOME", pwd->pw_dir, 1);
			_exit(system(run));
		} else {
			waitpid(pid, NULL, 0);
		}

		if(is_launch_x) {
			kill_x();
		}
	} while(is_launch_x);

	if(backup != NULL) free(backup);
}
