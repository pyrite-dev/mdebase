#include "mdm.h"

#include <X11/Xlib.h>

static pid_t x_server;
static int got_usr1;
static Display* display;

static void catch_usr1(int sig){
	got_usr1 = 1;
}

int launch_x(void){
	got_usr1 = 0;

	signal(SIGUSR1, catch_usr1);

	if((x_server = fork()) == 0){
		signal(SIGUSR1, SIG_IGN);
		execlp("X", "X", ":0", NULL);
		_exit(-1);
	}else{
		while(!got_usr1){
			if(waitpid(x_server, NULL, WNOHANG) != 0){
				fprintf(stderr, "server died. what happened?\n");
				return 1;
			}
			usleep(1000);
		}
	}

	setenv("DISPLAY", ":0", 1);

	return 0;
}

void init_x(void){
	XColor bg;

	display = XOpenDisplay(NULL);

	bg.red = 0x1c * 256;
	bg.green = 0x45 * 256;
	bg.blue = 0x73 * 256;
	XAllocColor(display, DefaultColormap(display, DefaultScreen(display)), &bg);

	XSetWindowBackground(display, DefaultRootWindow(display), bg.pixel);
	XClearWindow(display, DefaultRootWindow(display));
	XFlush(display);
}

int x_width(void){
	return DisplayWidth(display, DefaultScreen(display));
}

int x_height(void){
	return DisplayHeight(display, DefaultScreen(display));
}
