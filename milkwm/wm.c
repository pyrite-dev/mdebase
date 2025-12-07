#define _MILSKO
#include "milkwm.h"

#include <stb_ds.h>

static MwWidget root;

typedef struct wmframe {
	MwWidget titlebar;
	MwWidget title;

	MwWidget* left;
	MwWidget* right;

	MwLLPixmap maximize;
	MwLLPixmap iconify;

	int	dragging;
	MwPoint drag_point;
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
	int	   x;
	int	   i;

	MwVaApply(f->titlebar,
		  MwNx, MwDefaultBorderWidth(handle),
		  MwNy, MwDefaultBorderWidth(handle),
		  MwNwidth, ww - MwDefaultBorderWidth(handle) * 2,
		  MwNheight, TitleBarHeight + MwDefaultBorderWidth(handle) * 2,
		  NULL);

	x = MwDefaultBorderWidth(handle);
	for(i = 0; i < arrlen(f->left); i++) {
		MwVaApply(f->left[i],
			  MwNx, x,
			  NULL);
		x += TitleBarHeight;
	}

	x = MwGetInteger(f->titlebar, MwNwidth) - MwDefaultBorderWidth(handle);
	for(i = 0; i < arrlen(f->right); i++) {
		x -= TitleBarHeight;
		MwVaApply(f->right[i],
			  MwNx, x,
			  NULL);
	}

	MwVaApply(f->title,
		  MwNx, MwDefaultBorderWidth(f->titlebar) + arrlen(f->left) * TitleBarHeight,
		  MwNy, MwDefaultBorderWidth(f->titlebar),
		  MwNwidth, MwGetInteger(f->titlebar, MwNwidth) - MwDefaultBorderWidth(f->titlebar) * 2 - arrlen(f->left) * TitleBarHeight - arrlen(f->right) * TitleBarHeight,
		  MwNheight, MwGetInteger(f->titlebar, MwNheight) - MwDefaultBorderWidth(f->titlebar) * 2,
		  NULL);
}

static int alignment(const char* str) {
	if(strcmp(str, "Left") == 0) return MwALIGNMENT_BEGINNING;
	if(strcmp(str, "Right") == 0) return MwALIGNMENT_END;
	if(strcmp(str, "Center") == 0) return MwALIGNMENT_CENTER;

	return MwALIGNMENT_CENTER;
}

static void apply_config(MwWidget wnd) {
	wmframe_t*  f = wnd->opaque;
	const char* str;

	if(config_lookup_string(&wm_config, "Window.TitleBar.Align", &str)) {
		MwVaApply(f->title,
			  MwNalignment, alignment(str),
			  NULL);
	} else {
		MwVaApply(f->title,
			  MwNalignment, MwALIGNMENT_CENTER,
			  NULL);
	}
}

static void apply_button(MwWidget widget, const char* str) {
	MwWidget   w = widget;
	wmframe_t* f;
	MwLLPixmap px = NULL;

	while(w != NULL && strcmp(MwGetName(w), "frame") != 0) w = MwGetParent(w);

	f = w->opaque;

	if(strcmp(str, "Maximize") == 0) {
		px = f->maximize;
	} else if(strcmp(str, "Iconify") == 0) {
		px = f->iconify;
	}

	if(px != NULL) {
		int p = (MwGetInteger(widget, MwNwidth) - MwDefaultBorderWidth(widget) * 2 - px->common.width) / 2;
		MwVaApply(widget,
			  MwNpixmap, px,
			  MwNpadding, p,
			  NULL);
	}
}
static void drag_down(MwWidget handle, void* user, void* client) {
	MwWidget   w = handle;
	wmframe_t* f;
	int	   x = 0;
	int	   y = 0;
	MwPoint	   c;

	while(w != NULL && strcmp(MwGetName(w), "frame") != 0) w = MwGetParent(w);
	x += MwGetInteger(w, MwNx);
	y += MwGetInteger(w, MwNy);

	f = w->opaque;

	MwGetCursorCoord(w, &c);

	f->dragging	= 1;
	f->drag_point.x = c.x - x;
	f->drag_point.y = c.y - y;
}

static void drag_up(MwWidget handle, void* user, void* client) {
	MwWidget   w = handle;
	wmframe_t* f;

	while(w != NULL && strcmp(MwGetName(w), "frame") != 0) w = MwGetParent(w);

	f = w->opaque;

	f->dragging = 0;
}

static void drag(MwWidget handle, void* user, void* client) {
	MwWidget   w = handle;
	wmframe_t* f;

	while(w != NULL && strcmp(MwGetName(w), "frame") != 0) w = MwGetParent(w);

	f = w->opaque;

	if(f->dragging) {
		MwPoint c;

		MwGetCursorCoord(w, &c);

		MwVaApply(w,
			  MwNx, c.x - f->drag_point.x,
			  MwNy, c.y - f->drag_point.y,
			  NULL);
	}
}

MwWidget wm_frame(int w, int h) {
	MwWidget	  wnd, titlebar;
	int		  i;
	config_setting_t* s;
	const char*	  str;
	wmframe_t*	  f = malloc(sizeof(*f));
	unsigned char*	  icon;
	int		  y, x;

	pthread_mutex_lock(&xmutex);

	wnd	    = MwVaCreateWidget(MwWindowClass, "frame", root, MwDEFAULT, MwDEFAULT, w + MwDefaultBorderWidth(root) * 2, h + MwDefaultBorderWidth(root) * 4 + TitleBarHeight,
				       MwNhasBorder, 1,
				       MwNinverted, 0,
				       NULL);
	wnd->opaque = f;

	MwAddUserHandler(wnd, MwNresizeHandler, resize, NULL);

	f->dragging = 0;

	icon = malloc(6 * 6 * 4);

	memset(icon, 0, 6 * 6 * 4);
	for(y = 0; y < 6; y++) {
		for(x = 0; x < 6; x++) {
			if((y == 0 || y == 5 || x == 0 || x == 5) && !(2 <= y && y <= 3) && !(2 <= x && x <= 3)) icon[(y * 6 + x) * 4 + 3] = 255;
		}
	}
	f->iconify = MwLoadRaw(wnd, icon, 6, 6);

	memset(icon, 0, 6 * 6 * 4);
	for(y = 0; y < 6; y++) {
		for(x = 0; x < 6; x++) {
			if(y == 0 || y == 5 || x == 0 || x == 5) icon[(y * 6 + x) * 4 + 3] = 255;
		}
	}
	f->maximize = MwLoadRaw(wnd, icon, 6, 6);
	free(icon);

	f->left	 = NULL;
	f->right = NULL;

	f->titlebar = MwVaCreateWidget(MwFrameClass, "titlebar", wnd, 0, 0, 0, 0,
				       MwNhasBorder, 1,
				       MwNinverted, 1,
				       NULL);
	MwAddUserHandler(f->titlebar, MwNmouseUpHandler, drag_up, NULL);
	MwAddUserHandler(f->titlebar, MwNmouseMoveHandler, drag, NULL);
	MwAddUserHandler(f->titlebar, MwNmouseDownHandler, drag_down, NULL);

	f->title = MwCreateWidget(MwLabelClass, "title", f->titlebar, 0, 0, 0, 0);
	MwAddUserHandler(f->title, MwNmouseUpHandler, drag_up, NULL);
	MwAddUserHandler(f->title, MwNmouseMoveHandler, drag, NULL);
	MwAddUserHandler(f->title, MwNmouseDownHandler, drag_down, NULL);

	s = config_lookup(&wm_config, "Window.TitleBar.Buttons.Left");
	for(i = 0; (str = config_setting_get_string_elem(s, i)) != NULL; i++) {
		MwWidget w = MwVaCreateWidget(MwButtonClass, "button", f->titlebar, 0, MwDefaultBorderWidth(wnd), TitleBarHeight, TitleBarHeight,
					      MwNborderWidth, 1,
					      NULL);

		apply_button(w, str);

		arrput(f->left, w);
	}

	s = config_lookup(&wm_config, "Window.TitleBar.Buttons.Right");
	for(i = 0; (str = config_setting_get_string_elem(s, i)) != NULL; i++) {
		MwWidget w = MwVaCreateWidget(MwButtonClass, "button", f->titlebar, 0, MwDefaultBorderWidth(wnd), TitleBarHeight, TitleBarHeight,
					      MwNborderWidth, 1,
					      NULL);

		apply_button(w, str);

		arrput(f->right, w);
	}

	apply_config(wnd);

	resize(wnd, NULL, NULL);

	pthread_mutex_unlock(&xmutex);

	return wnd;
}

void wm_destroy(MwWidget widget) {
	wmframe_t* f = widget->opaque;

	pthread_mutex_lock(&xmutex);
	arrfree(f->left);
	arrfree(f->right);

	MwLLDestroyPixmap(f->maximize);
	MwLLDestroyPixmap(f->iconify);
	free(widget->opaque);
	MwDestroyWidget(widget);
	pthread_mutex_unlock(&xmutex);
}

void wm_set_name(MwWidget widget, const char* name) {
	wmframe_t* f = widget->opaque;
	char*	   s = MDEStringConcatenate(" ", name);

	pthread_mutex_lock(&xmutex);
	MwVaApply(f->title,
		  MwNtext, s,
		  NULL);
	pthread_mutex_unlock(&xmutex);

	free(s);
}

void wm_focus(MwWidget widget, int focus) {
	wmframe_t*  f  = widget->opaque;
	const char* bg = NULL;
	const char* fg = NULL;

	pthread_mutex_lock(&xmutex);
	if(focus) {
		config_lookup_string(&wm_config, "Window.TitleBar.Active.Background", &bg);
		config_lookup_string(&wm_config, "Window.TitleBar.Active.Foreground", &fg);
	} else {
		config_lookup_string(&wm_config, "Window.TitleBar.Inactive.Background", &bg);
		config_lookup_string(&wm_config, "Window.TitleBar.Inactive.Foreground", &fg);
	}

	MwVaApply(f->title,
		  MwNbackground, bg,
		  MwNforeground, fg,
		  NULL);

	pthread_mutex_unlock(&xmutex);
}
