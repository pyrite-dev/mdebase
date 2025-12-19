#define _MILSKO
#include "milkwm.h"

#include <stb_ds.h>

MwWidget root	   = NULL;
int	 wm_rehash = 0;

typedef struct wmframe {
	MwWidget inside;
	MwWidget titlebar;
	MwWidget title;

	MwWidget* left;
	MwWidget* right;

	MwLLPixmap maximize;
	MwLLPixmap restore;
	MwLLPixmap iconify;

	int	dragging;
	MwPoint drag_point;

	MwLLPixmap background;

	int	maximized;
	MwRect	old_size;
	MwPoint old_point;
} wmframe_t;

void loop_wm(void) {
	void* out;

	pthread_mutex_lock(&xmutex);

	MwLibraryInit();

	root = MwVaCreateWidget(NULL, "root", NULL, 0, 0, 0, 0,
				COMMON_LOOK,
				NULL);

	pthread_mutex_unlock(&xmutex);

	while(1) {
		int s = 0;

		pthread_mutex_lock(&xmutex);
		if(wm_rehash) {
			wm_config_init();
			wm_config_read();
			set_background_x();
			wm_rehash = 0;
		}

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

	MwVaApply(f->inside,
		  MwNwidth, ww - MwDefaultBorderWidth(handle) * 2 - Gap * 2,
		  MwNheight, wh - MwDefaultBorderWidth(handle) * 2 - Gap * 2,
		  NULL);

	MwVaApply(f->titlebar,
		  MwNx, MwDefaultBorderWidth(handle),
		  MwNy, MwDefaultBorderWidth(handle),
		  MwNwidth, ww - MwDefaultBorderWidth(handle) * 4 - Gap * 2,
		  MwNheight, TitleBarHeight,
		  NULL);

	x = 0;
	for(i = 0; i < arrlen(f->left); i++) {
		MwVaApply(f->left[i],
			  MwNx, x,
			  NULL);
		x += TitleBarHeight;
	}

	x = MwGetInteger(f->titlebar, MwNwidth);
	for(i = 0; i < arrlen(f->right); i++) {
		x -= TitleBarHeight;
		MwVaApply(f->right[i],
			  MwNx, x,
			  NULL);
	}

	MwVaApply(f->title,
		  MwNx, arrlen(f->left) * TitleBarHeight,
		  MwNy, 0,
		  MwNwidth, MwGetInteger(f->titlebar, MwNwidth) - arrlen(f->left) * TitleBarHeight - arrlen(f->right) * TitleBarHeight,
		  MwNheight, MwGetInteger(f->titlebar, MwNheight),
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

static void maximize(MwWidget handle, void* user, void* client) {
	wmframe_t* f = ((MwWidget)user)->opaque;
	MwRect	   rc;

	set_focus_x(user);

	if(f->maximized) {
		f->maximized = 0;

		MwVaApply(handle,
			  MwNpixmap, f->maximize,
			  NULL);

		MwVaApply(user,
			  MwNx, f->old_point.x,
			  MwNy, f->old_point.y,
			  MwNwidth, f->old_size.width,
			  MwNheight, f->old_size.height,
			  NULL);
	} else {
		f->maximized = 1;

		f->old_point.x	   = MwGetInteger(user, MwNx);
		f->old_point.y	   = MwGetInteger(user, MwNy);
		f->old_size.width  = MwGetInteger(user, MwNwidth);
		f->old_size.height = MwGetInteger(user, MwNheight);

		MwGetScreenSize(user, &rc);

		MwVaApply(handle,
			  MwNpixmap, f->restore,
			  NULL);

		MwVaApply(user,
			  MwNx, 0,
			  MwNy, 0,
			  MwNwidth, rc.width,
			  MwNheight, rc.height,
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
		MwAddUserHandler(widget, MwNactivateHandler, maximize, w);
	} else if(strcmp(str, "Iconify") == 0) {
		px = f->iconify;
	}

	if(px != NULL) {
		MwVaApply(widget,
			  MwNpixmap, px,
			  MwNfillArea, 0,
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

	set_focus_x(w);
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
	if(f->maximized) return;

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

	wnd	    = MwVaCreateWidget(MwWindowClass, "frame", root, MwDEFAULT, MwDEFAULT, wm_entire_width(w), wm_entire_height(h),
				       MwNhasBorder, 1,
				       MwNinverted, 0,
				       NULL);
	wnd->opaque = f;

	MwAddUserHandler(wnd, MwNresizeHandler, resize, NULL);

	f->background = NULL;
	f->maximized  = 0;
	f->dragging   = 0;

	icon = malloc(10 * 6 * 4);

	memset(icon, 0, 10 * 6 * 4);
	for(y = 0; y < 6; y++) {
		for(x = 0; x < 6; x++) {
			if(y == 0 || y == 5 || x == 0 || x == 5) icon[(y * 6 + x) * 4 + 3] = 255;
		}
	}
	f->maximize = MwLoadRaw(wnd, icon, 6, 6);

	memset(icon, 0, 10 * 6 * 4);
	for(y = 0; y < 6; y++) {
		for(x = 0; x < 10; x++) {
			if((y == 0 || y == 5 || x == 2 || x == 7 || x == 0 || x == 9) && x != 1 && x != 8) icon[(y * 10 + x) * 4 + 3] = 255;
		}
	}
	f->restore = MwLoadRaw(wnd, icon, 10, 6);

	memset(icon, 0, 10 * 6 * 4);
	for(y = 0; y < 6; y++) {
		for(x = 0; x < 6; x++) {
			if((y == 0 || y == 5 || x == 0 || x == 5) && !(2 <= y && y <= 3) && !(2 <= x && x <= 3)) icon[(y * 6 + x) * 4 + 3] = 255;
		}
	}
	f->iconify = MwLoadRaw(wnd, icon, 6, 6);
	free(icon);

	f->left	 = NULL;
	f->right = NULL;

	f->inside = MwVaCreateWidget(MwFrameClass, "inside", wnd, MwDefaultBorderWidth(wnd) + Gap, MwDefaultBorderWidth(wnd) + Gap, 0, 0,
				     MwNhasBorder, 1,
				     MwNinverted, 1,
				     NULL);

	f->titlebar = MwCreateWidget(MwFrameClass, "titlebar", f->inside, 0, 0, 0, 0);
	MwAddUserHandler(f->titlebar, MwNmouseUpHandler, drag_up, NULL);
	MwAddUserHandler(f->titlebar, MwNmouseMoveHandler, drag, NULL);
	MwAddUserHandler(f->titlebar, MwNmouseDownHandler, drag_down, NULL);

	f->title = MwCreateWidget(MwLabelClass, "title", f->titlebar, 0, 0, 0, 0);
	MwAddUserHandler(f->title, MwNmouseUpHandler, drag_up, NULL);
	MwAddUserHandler(f->title, MwNmouseMoveHandler, drag, NULL);
	MwAddUserHandler(f->title, MwNmouseDownHandler, drag_down, NULL);

	s = config_lookup(&wm_config, "Window.TitleBar.Buttons.Left");
	for(i = 0; (str = config_setting_get_string_elem(s, i)) != NULL; i++) {
		MwWidget w = MwVaCreateWidget(MwButtonClass, "button", f->titlebar, 0, 0, TitleBarHeight, TitleBarHeight,
					      MwNborderWidth, 1,
					      NULL);

		apply_button(w, str);

		arrput(f->left, w);
	}

	s = config_lookup(&wm_config, "Window.TitleBar.Buttons.Right");
	for(i = 0; (str = config_setting_get_string_elem(s, i)) != NULL; i++) {
		MwWidget w = MwVaCreateWidget(MwButtonClass, "button", f->titlebar, 0, 0, TitleBarHeight, TitleBarHeight,
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

	if(f->background != NULL) MwLLDestroyPixmap(f->background);
	MwLLDestroyPixmap(f->maximize);
	MwLLDestroyPixmap(f->restore);
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

static char tmp_bg[64];
MwLLPixmap  lookup_bg(MwWidget widget, const char* path, const char** bg) {
	 config_setting_t* s  = config_lookup(&wm_config, path);
	 wmframe_t*	   f  = widget->opaque;
	 MwLLPixmap	   px = NULL;

	 *bg = NULL;
	 if(s == NULL) {
	 } else if(config_setting_type(s) == CONFIG_TYPE_STRING) {
		 *bg = config_setting_get_string(s);
	 } else if(config_setting_type(s) == CONFIG_TYPE_GROUP) {
		 const char*	from;
		 const char*	to;
		 int		i;
		 unsigned char* d = malloc(1 * TitleBarHeight * 4);
		 MwLLColor	cfrom;
		 MwLLColor	cto;
		 int		fr, fg, fb;
		 int		tr, tg, tb;

		 config_setting_lookup_string(s, "From", &from);
		 config_setting_lookup_string(s, "To", &to);

		 cfrom = MwParseColor(widget, from == NULL ? MwGetText(widget, MwNbackground) : from);
		 cto   = MwParseColor(widget, to == NULL ? MwGetText(widget, MwNbackground) : to);
		 MwColorGet(cfrom, &fr, &fg, &fb);
		 MwColorGet(cto, &tr, &tg, &tb);
		 MwLLFreeColor(cfrom);
		 MwLLFreeColor(cto);

		 memset(d, 255, 1 * TitleBarHeight * 4);

		 for(i = 0; i < TitleBarHeight; i++) {
			 d[i * 4 + 0] = fr + (tr - fr) * i / TitleBarHeight;
			 d[i * 4 + 1] = fg + (tg - fg) * i / TitleBarHeight;
			 d[i * 4 + 2] = fb + (tb - fb) * i / TitleBarHeight;
		 }
		 px = MwLoadRaw(widget, d, 1, TitleBarHeight);

		 sprintf(tmp_bg, "#%02x%02x%02x", fr + (tr - fr) / 2, fg + (tg - fg) / 2, fb + (tb - fb) / 2);
		 *bg = &tmp_bg[0];

		 free(d);
	 }

	 return px;
}

void wm_focus(MwWidget widget, int focus) {
	wmframe_t*  f	 = widget->opaque;
	const char* bg	 = NULL;
	const char* fg	 = NULL;
	MwLLPixmap  bgpx = NULL;

	pthread_mutex_lock(&xmutex);
	if(focus) {
		bgpx = lookup_bg(widget, "Window.TitleBar.Active.Background", &bg);
		config_lookup_string(&wm_config, "Window.TitleBar.Active.Foreground", &fg);
	} else {
		bgpx = lookup_bg(widget, "Window.TitleBar.Inactive.Background", &bg);
		config_lookup_string(&wm_config, "Window.TitleBar.Inactive.Foreground", &fg);
	}

	if(f->background != NULL) MwLLDestroyPixmap(f->background);
	f->background = bgpx;

	MwVaApply(f->title,
		  MwNbackground, bg,
		  MwNbackgroundPixmap, bgpx,
		  MwNforeground, fg,
		  NULL);

	pthread_mutex_unlock(&xmutex);
}

MwWidget wm_get_inside(MwWidget widget) {
	MwWidget ret;

	pthread_mutex_lock(&xmutex);
	ret = ((wmframe_t*)widget->opaque)->inside;
	pthread_mutex_unlock(&xmutex);

	return ret;
}

int wm_entire_width(int content) {
	return content + MwDefaultBorderWidth(root) * 4 + Gap * 2;
}

int wm_entire_height(int content) {
	return content + MwDefaultBorderWidth(root) * 4 + Gap * 2 + TitleBarHeight;
}

int wm_content_width(int entire) {
	return entire - wm_entire_width(0);
}

int wm_content_height(int entire) {
	return entire - wm_entire_height(0);
}

int wm_content_x(void) {
	return MwDefaultBorderWidth(root);
}

int wm_content_y(void) {
	return MwDefaultBorderWidth(root) + TitleBarHeight;
}
