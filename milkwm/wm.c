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

	MwWidget menu_button;

	MwLLPixmap maximize;
	MwLLPixmap restore;
	MwLLPixmap iconify;

	int	dragging;
	MwPoint drag_point;

	MwLLPixmap background;

	int	maximized;
	MwRect	old_size;
	MwPoint old_point;

	MwWidget submenu;

	MwMenu menu;

	long tick;
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
	wmframe_t*  f	= wnd->opaque;
	const char* str = NULL;
	xl_node_t*  n	= wm_config->root->first_child;

	while(n != NULL) {
		if(n->type == XL_NODE_NODE && strcmp(n->name, "Window") == 0) {
			xl_node_t* n2 = n->first_child;
			while(n2 != NULL) {
				if(n2->type == XL_NODE_NODE && strcmp(n2->name, "Align") == 0 && n2->text != NULL) {
					str = n2->text;
				}
				n2 = n2->next;
			}
		}
		n = n->next;
	}

	if(str != NULL) {
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
	int	   i;

	set_focus_x(user);

	if(f->maximized) {
		for(i = 0; i < arrlen(f->menu->sub); i++) {
			if(strcmp(f->menu->sub[i]->name, "Restore") == 0) {
				free(f->menu->sub[i]->name);
				f->menu->sub[i]->name = MwStringDuplicate("Maximize");
			}
		}

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
		for(i = 0; i < arrlen(f->menu->sub); i++) {
			if(strcmp(f->menu->sub[i]->name, "Maximize") == 0) {
				free(f->menu->sub[i]->name);
				f->menu->sub[i]->name = MwStringDuplicate("Restore");
			}
		}

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
			  MwNheight, rc.height - 46,
			  NULL);
	}
}

static void spawn_menu(MwWidget handle, void* user, void* client) {
	MwWidget   w = handle;
	wmframe_t* f;
	MwPoint	   p;
	long	   t;

	while(w != NULL && strcmp(MwGetName(w), "frame") != 0) w = MwGetParent(w);

	f = w->opaque;

	if(f->submenu == NULL) {
		f->submenu = MwCreateWidget(MwSubMenuClass, "submenu", handle, 0, MwGetInteger(handle, MwNheight), 0, 0);

		p.x = 0;
		p.y = MwGetInteger(handle, MwNheight);
		MwSubMenuAppear(f->submenu, f->menu, &p, 0);

		MwVaApply(f->menu_button,
			  MwNforceInverted, 1,
			  NULL);
	} else {
		MwDestroyWidget(f->submenu);
		f->submenu = NULL;

		MwVaApply(f->menu_button,
			  MwNforceInverted, 0,
			  NULL);
	}

	if(((t = MwTimeGetTick()) - f->tick) < MwDoubleClickTimeout) {
		delete_x(w);
	}

	f->tick = t;
}

static void menu_menu(MwWidget handle, void* user, void* client) {
	MwWidget   w = handle;
	wmframe_t* f;
	int	   i;
	MwMenu	   m = client;

	while(w != NULL && strcmp(MwGetName(w), "frame") != 0) w = MwGetParent(w);

	f = w->opaque;

	if(strcmp(m->name, "Maximize") == 0 || strcmp(m->name, "Restore") == 0) {
		for(i = 0; i < arrlen(f->left); i++) {
			MwLLPixmap px = MwGetVoid(f->left[i], MwNpixmap);

			if(px == f->maximize || px == f->restore) maximize(f->left[i], w, NULL);
		}

		for(i = 0; i < arrlen(f->right); i++) {
			MwLLPixmap px = MwGetVoid(f->right[i], MwNpixmap);

			if(px == f->maximize || px == f->restore) maximize(f->right[i], w, NULL);
		}
	} else if(strcmp(m->name, "Close") == 0) {
		delete_x(w);
	}

	f->submenu = NULL;
	MwVaApply(f->menu_button,
		  MwNforceInverted, 0,
		  NULL);
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
	} else if(strcmp(str, "Menu") == 0) {
		f->menu_button = widget;
		MwAddUserHandler(widget, MwNactivateHandler, spawn_menu, w);
		MwAddUserHandler(widget, MwNmenuHandler, menu_menu, w);
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

	if(f->submenu != NULL) {
		MwDestroyWidget(f->submenu);
		f->submenu = NULL;

		MwVaApply(f->menu_button,
			  MwNforceInverted, 0,
			  NULL);
	}

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

static xl_node_t* get_window(void) {
	xl_node_t* n;

	if(wm_config == NULL || wm_config->root == NULL) return NULL;

	n = wm_config->root->first_child;
	while(n != NULL) {
		if(n->type == XL_NODE_NODE && strcmp(n->name, "Window") == 0) {
			return n;
		}

		n = n->next;
	}

	return NULL;
}

static xl_node_t* get_titlebar(void) {
	xl_node_t* n;

	if(wm_config == NULL || wm_config->root == NULL) return NULL;

	n = get_window();
	if(n == NULL) return NULL;

	n = n->first_child;
	while(n != NULL) {
		if(n->type == XL_NODE_NODE && strcmp(n->name, "TitleBar") == 0) {
			return n;
		}

		n = n->next;
	}

	return NULL;
}

MwWidget wm_frame(int w, int h) {
	MwWidget       wnd, titlebar;
	int	       i;
	wmframe_t*     f = malloc(sizeof(*f));
	unsigned char* icon;
	int	       y, x;
	MwMenu	       m;
	const char*    menus[] = {
	       "Maximize",
	       "Iconify",
	       "----",
	       "Close",
	       NULL};

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

	f->submenu = NULL;

	f->menu		= malloc(sizeof(*f->menu));
	f->menu->name	= NULL;
	f->menu->keep	= 0;
	f->menu->wsub	= NULL;
	f->menu->sub	= NULL;
	f->menu->opaque = NULL;

	for(i = 0; menus[i] != NULL; i++) {
		m	  = malloc(sizeof(*m));
		m->name	  = MwStringDuplicate(menus[i]);
		m->keep	  = 0;
		m->wsub	  = NULL;
		m->sub	  = NULL;
		m->opaque = NULL;
		arrput(f->menu->sub, m);
	}

	f->menu_button = NULL;

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

	f->tick = 0;

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

	f->title = MwVaCreateWidget(MwLabelClass, "title", f->titlebar, 0, 0, 0, 0,
				    MwNleftPadding, 3,
				    NULL);
	MwAddUserHandler(f->title, MwNmouseUpHandler, drag_up, NULL);
	MwAddUserHandler(f->title, MwNmouseMoveHandler, drag, NULL);
	MwAddUserHandler(f->title, MwNmouseDownHandler, drag_down, NULL);

	for(i = 0; i < 2; i++) {
		xl_node_t* node = get_titlebar();
		xl_node_t* n;

		if(node == NULL) continue;

		n = node->first_child;
		while(n != NULL) {
			if(n->type == XL_NODE_NODE && strcmp(n->name, "Button") == 0) {
				xl_node_t* n2 = n->first_child;
				while(n2 != NULL) {
					if(n2->type == XL_NODE_NODE && strcmp(n2->name, i == 0 ? "Left" : "Right") == 0) {
						xl_node_t* n3 = n2->first_child;
						while(n3 != NULL) {
							if(n3->type == XL_NODE_NODE) {
								MwWidget w = MwVaCreateWidget(MwButtonClass, "button", f->titlebar, 0, 0, TitleBarHeight, TitleBarHeight,
											      MwNborderWidth, 1,
											      NULL);

								apply_button(w, n3->name);

								if(i == 0) {
									arrput(f->left, w);
								} else {
									arrput(f->right, w);
								}
							}

							n3 = n3->next;
						}
					}

					n2 = n2->next;
				}
			}

			n = n->next;
		}
	}

	apply_config(wnd);

	resize(wnd, NULL, NULL);

	pthread_mutex_unlock(&xmutex);

	wm_set_icon_by_name(wnd, NULL, "unknown");

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

	pthread_mutex_lock(&xmutex);
	MwVaApply(f->title,
		  MwNtext, name,
		  NULL);
	pthread_mutex_unlock(&xmutex);
}

int wm_set_icon_by_name(MwWidget widget, const char* cat, const char* name) {
	wmframe_t* f  = widget->opaque;
	MwLLPixmap px = NULL;
	int	   st = 0;

	pthread_mutex_lock(&xmutex);
	if(f->menu_button != NULL) {
		char* p;

		if((px = MwGetVoid(f->menu_button, MwNpixmap)) != NULL) {
			MwLLDestroyPixmap(px);
			px = NULL;
		}

		if((p = MDEIconLookUp(cat == NULL ? "Applications" : cat, name, 16)) != NULL) {
			px = MwLoadImage(f->menu_button, p);

			free(p);
		}

		MwSetVoid(f->menu_button, MwNpixmap, px);
	}

	pthread_mutex_unlock(&xmutex);

	if(px != NULL) st = 1;

	return st;
}

void wm_set_icon(MwWidget widget, unsigned char* data, int width, int height) {
	wmframe_t* f  = widget->opaque;
	MwLLPixmap px = NULL;

	pthread_mutex_lock(&xmutex);
	if(f->menu_button != NULL) {
		if((px = MwGetVoid(f->menu_button, MwNpixmap)) != NULL) {
			MwLLDestroyPixmap(px);
			px = NULL;
		}

		px = MwLoadRaw(f->menu_button, data, width, height);

		MwSetVoid(f->menu_button, MwNpixmap, px);
	}
	pthread_mutex_unlock(&xmutex);
}

static char tmp_bg[64];

MwLLPixmap lookup_col(MwWidget widget, const char* bgfg, const char* type, const char** bg) {
	wmframe_t* f	= widget->opaque;
	MwLLPixmap px	= NULL;
	xl_node_t* node = get_window();
	xl_node_t* n;

	*bg = NULL;

	if(node == NULL) return NULL;

	n = node->first_child;

	while(n != NULL) {
		if(n->type == XL_NODE_NODE && strcmp(n->name, bgfg) == 0) {
			xl_node_t* n2 = n->first_child;
			while(n2 != NULL) {
				if(n2->type == XL_NODE_NODE && strcmp(n2->name, type) == 0) {
					if(n2->text != NULL) {
						*bg = n2->text;
					} else {
						const char*    from = NULL;
						const char*    to;
						int	       i;
						unsigned char* d = malloc(1 * TitleBarHeight * 4);
						MwLLColor      cfrom;
						MwLLColor      cto;
						int	       fr, fg, fb;
						int	       tr, tg, tb;
						xl_node_t*     n3 = n2->first_child;

						while(n3 != NULL) {
							if(n3->type == XL_NODE_NODE && strcmp(n3->name, "Gradient") == 0) {
								xl_attribute_t* a = n3->first_attribute;

								while(a != NULL) {
									if(strcmp(a->key, "From") == 0) {
										from = a->value;
									} else if(strcmp(a->key, "To") == 0) {
										to = a->value;
									}
									a = a->next;
								}
							}
							n3 = n3->next;
						}

						if(from == NULL || to == NULL) {
							free(d);

							n2 = n2->next;
							continue;
						}

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
				}
				n2 = n2->next;
			}
		}

		n = n->next;
	}

	return px;
}

void wm_focus(MwWidget widget, int focus) {
	wmframe_t*  f	 = widget->opaque;
	const char* bg	 = NULL;
	const char* fg	 = NULL;
	MwLLPixmap  bgpx = NULL;
	MwLLPixmap  fgpx = NULL;

	pthread_mutex_lock(&xmutex);
	if(focus) {
		bgpx = lookup_col(widget, "Background", "Active", &bg);
		fgpx = lookup_col(widget, "Foreground", "Active", &fg);
	} else {
		bgpx = lookup_col(widget, "Background", "Inactive", &bg);
		fgpx = lookup_col(widget, "Foreground", "Inactive", &fg);
	}

	if(fgpx != NULL) MwLLDestroyPixmap(fgpx);

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
