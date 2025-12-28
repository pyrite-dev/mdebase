#define _MILSKO
#include "mpanel.h"

MwWidget window, box;

static int first = 1;

static void sig_hup(int sig) {
	reload_modules();
}

static void resize(MwWidget handle, void* user, void* client) {
	int bw = MwDefaultBorderWidth(window) + 2;

	MwVaApply(box,
		  MwNx, bw,
		  MwNy, bw,
		  MwNwidth, MwGetInteger(window, MwNwidth) - bw * 2,
		  MwNheight, MwGetInteger(window, MwNheight) - bw * 2,
		  NULL);

	if(first && MwGetInteger(window, MwNheight) > 32) {
		first = 0;

		load_modules();

		signal(SIGHUP, sig_hup);
	}
}

int main() {
	MwRect rc;

	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP, SIG_IGN);

	MwLibraryInit();

	window = MwVaCreateWidget(MwWindowClass, "window", NULL, MwDEFAULT, MwDEFAULT, 0, 0,
				  MwNtitle, "mpanel",
				  MwNhasBorder, 1,
				  MwNinverted, 0,
				  COMMON_LOOK,
				  NULL);

	box = MwCreateWidget(MwBoxClass, "box", window, 0, 0, 0, 0);

	MwAddUserHandler(window, MwNresizeHandler, resize, NULL);

	MwGetScreenSize(window, &rc);

	MwLLBeginStateChange(window->lowlevel);
	MwVaApply(window,
		  MwNx, 0,
		  MwNy, rc.height - 46,
		  MwNwidth, rc.width,
		  MwNheight, 46,
		  NULL);
	MwLLMakeToolWindow(window->lowlevel);
	MwLLEndStateChange(window->lowlevel);

	MwLoop(window);
}
