#include "mdm.h"
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int is_launch_x = 1;
int is_daemon	= 1;

gid_t		       gid;
uid_t		       uid;
session_environment_t* env;

static void set_env_standard(struct passwd* pwd);
static void set_env_xdg(struct passwd* pwd);

int main(int argc, char** argv) {
	int   i, st, exit;
	char* backup = NULL;
	pid_t pid;

	for(i = 1; i < argc; i++) {
		if(argv[i][0] == '-') {
			if(strcmp(argv[i], "-X") == 0 || strcmp(argv[i], "--no-launch-x") == 0) {
				is_launch_x = 0;
			} else if(strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--no-daemon") == 0) {
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

	if(is_daemon && (pid = fork()) != 0) {
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

		if(env->run == NULL && env->try_run == NULL) continue;

		if((pid = fork()) == 0) {
			struct passwd* pwd = getpwuid(uid);

			setgid(gid);
			setuid(uid);
			chdir(pwd->pw_dir);

			set_env_standard(pwd);
			set_env_xdg(pwd);

			if((exit = system(env->try_run)) != 0) {
				exit = system(env->run);
			} else {
				_exit(exit);
			}
		} else {
			waitpid(pid, NULL, 0);
		}

		if(is_launch_x) {
			unsetenv("DISPLAY");
			kill_x();
		}
	} while(is_launch_x);

	if(backup != NULL) free(backup);
}

void set_env_standard(struct passwd* pwd) {
	setenv("HOME", pwd->pw_dir, 1);
	setenv("PWD", pwd->pw_dir, 1);
	setenv("USER", pwd->pw_name, 1);
	setenv("SHELL", pwd->pw_shell, 1);
	setenv("LOGNAME", pwd->pw_shell, 1);
}
void set_env_xdg(struct passwd* pwd) {
	// this directory isn't avaliable on bsd, but we can't check for all of them.
	// so just assume it's only on linux, idek if that's true but who care
#if defined(__linux__)
	char runtime_dir[255];
	snprintf(runtime_dir, 255, "/run/user/%d", pwd->pw_uid);
	setenv("XDG_RUNTIME_DIR", runtime_dir, 1);
#endif

	switch(env->session_type) {
	case MDM_SESSION_TYPE_X:
		setenv("XDG_SESSION_TYPE", "x11", 1);
		break;
	case MDM_SESSION_TYPE_WAYLAND:
		setenv("XDG_SESSION_TYPE", "wayland", 1);
		break;
	}

	if(env->desktop_names) {
		if(strlen(env->desktop_names) > 1) {
			setenv("XDG_SESSION_DESKTOP", strtok(env->desktop_names, ";"), 1);
		}
		setenv("XDG_CURRENT_DESKTOP", env->desktop_names, 1);
	}

	setenv("XDG_SESSION_CLASS", "user", 1);
	setenv("XDG_SESSION_ID", "1", 1);
	setenv("XDG_SEAT", "seat0", 1);
	// setenv("XDG_VTNR", "/dev/pts/7", 1);
}
