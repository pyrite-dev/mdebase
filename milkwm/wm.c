#define _MILSKO
#include "milkwm.h"

static MwWidget root;

typedef struct wmframe {
	MwWidget titlebar;
} wmframe_t;

void loop_wm(void) {
	void* out;

	pthread_mutex_lock(&xmutex);

	MwLibraryInit();

	root = MwCreateWidget(NULL, "root", NULL, 0, 0, 0, 0);

	pthread_mutex_unlock(&xmutex);

	while(1) {
		int s = 0;

		pthread_mutex_lock(&xmutex);
		while(MwPending(root)) {
			if((s = MwStep(root)) != 0) break;
		}
		pthread_mutex_unlock(&xmutex);
		if(s != 0) break;

		MwTimeSleep(30);
	}

	pthread_join(xthread, &out);
}

static void resize(MwWidget handle, void* user, void* client) {
	wmframe_t* f  = handle->opaque;
	int	   ww = MwGetInteger(handle, MwNwidth);
	int	   wh = MwGetInteger(handle, MwNheight);

	MwVaApply(f->titlebar,
		  MwNx, MwDefaultBorderWidth(handle),
		  MwNy, MwDefaultBorderWidth(handle),
		  MwNwidth, ww - MwDefaultBorderWidth(handle) * 2,
		  MwNheight, TitleBarHeight + MwDefaultBorderWidth(handle) * 2,
		  NULL);
}

MwWidget wm_frame(int w, int h) {
	MwWidget   wnd, titlebar;
	int	   i;
	wmframe_t* f = malloc(sizeof(*f));

	pthread_mutex_lock(&xmutex);

	wnd	    = MwVaCreateWidget(MwWindowClass, "frame", root, MwDEFAULT, MwDEFAULT, w + MwDefaultBorderWidth(root) * 2, h + MwDefaultBorderWidth(root) * 4 + TitleBarHeight,
				       MwNhasBorder, 1,
				       MwNinverted, 0,
				       NULL);
	wnd->opaque = f;

	f->titlebar = MwVaCreateWidget(MwFrameClass, "titlebar", wnd, 0, 0, 0, 0,
				       MwNhasBorder, 1,
				       MwNinverted, 1,
				       NULL);

	MwCreateWidget(MwButtonClass, "button", f->titlebar, MwDefaultBorderWidth(wnd), MwDefaultBorderWidth(wnd), TitleBarHeight, TitleBarHeight);

	resize(wnd, NULL, NULL);

	pthread_mutex_unlock(&xmutex);

	return wnd;
}

void wm_set_name(MwWidget widget, const char* name) {
}
