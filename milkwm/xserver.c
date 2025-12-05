#define _MILSKO
#define USE_X11
#include "milkwm.h"

#include <stb_ds.h>

Display*	xdisplay;
pthread_t	xthread;
pthread_mutex_t xmutex;

static void* old_handler;
static int   error_happened = 0;
static int   ignore_error(Display* disp, XErrorEvent* ev) {
	  error_happened = 1;
	  return 0;
}

static void begin_error(void) {
	old_handler    = XSetErrorHandler(ignore_error);
	error_happened = 0;
}

static void end_error(void) {
	XSync(xdisplay, False);
	XSetErrorHandler(old_handler);
}

static void* x11_thread_routine(void* arg) {
	loop_x();
	XCloseDisplay(xdisplay);
	return NULL;
}

static Window focus = None, nofocus;
static void   set_focus(Window w) {
	  if(w == None) {
		  XSetInputFocus(xdisplay, nofocus, RevertToParent, CurrentTime);
	  } else {
		  XSetInputFocus(xdisplay, w, RevertToParent, CurrentTime);
	  }
	  focus = w;
}

int init_x(void) {
	void*		     old;
	XSetWindowAttributes xswa;

	xswa.override_redirect = True;

	xdisplay = XOpenDisplay(NULL);

	begin_error();
	XSelectInput(xdisplay, DefaultRootWindow(xdisplay), SubstructureRedirectMask);
	end_error();

	if(error_happened) return 1;

	nofocus = XCreateSimpleWindow(xdisplay, DefaultRootWindow(xdisplay), -10, -10, 1, 1, 0, 0, 0);
	XChangeWindowAttributes(xdisplay, nofocus, CWOverrideRedirect, &xswa);
	XMapWindow(xdisplay, nofocus);
	set_focus(None);

	XChangeProperty(xdisplay, DefaultRootWindow(xdisplay), XInternAtom(xdisplay, "_NET_SUPPORTING_WM_CHECK", False), XA_WINDOW, 32, PropModeReplace, (unsigned char*)&nofocus, 1);
	XChangeProperty(xdisplay, nofocus, XInternAtom(xdisplay, "_NET_SUPPORTING_WM_CHECK", False), XA_WINDOW, 32, PropModeReplace, (unsigned char*)&nofocus, 1);
	XChangeProperty(xdisplay, nofocus, XInternAtom(xdisplay, "_NET_WM_NAME", False), XInternAtom(xdisplay, "UTF8_STRING", False), 8, PropModeReplace, (unsigned char*)"milkwm", 6);

	pthread_mutex_init(&xmutex, NULL);

	pthread_create(&xthread, NULL, x11_thread_routine, NULL);

	return 0;
}

typedef struct window {
	MwWidget frame;
	Window	 client;
} window_t;
static window_t* windows = NULL;

static void save(Window w) {
	XSelectInput(xdisplay, w, StructureNotifyMask | FocusChangeMask | PropertyChangeMask);
	XSetWindowBorderWidth(xdisplay, w, 0);
	XAddToSaveSet(xdisplay, w);
}

static int parent_eq(Window wnd, Window might_be_parent) {
	Window	     root, parent;
	Window*	     children;
	unsigned int nchildren;

	XQueryTree(xdisplay, wnd, &root, &parent, &children, &nchildren);
	if(children != NULL) XFree(children);

	if(parent == might_be_parent) return 1;
	if(parent != root) return parent_eq(parent, might_be_parent);
	return 0;
}

static void set_name(Window wnd) {
	Atom	       type;
	int	       format;
	unsigned long  nitem, after;
	unsigned char* buf;
	Atom	       atom = XInternAtom(xdisplay, "WM_NAME", False);
	int	       i;

	for(i = 0; i < arrlen(windows); i++) {
		if(windows[i].client == wnd) {
			if(XGetWindowProperty(xdisplay, wnd, atom, 0, 1024, False, XA_STRING, &type, &format, &nitem, &after, &buf) == Success) {
				wm_set_name(windows[i].frame, buf);
				XFree(buf);
			}
		}
	}
}

void loop_x(void) {
	XEvent	     ev;
	Window	     root, parent;
	Window*	     children;
	unsigned int nchildren;
	int	     i;

	begin_error();

	XQueryTree(xdisplay, DefaultRootWindow(xdisplay), &root, &parent, &children, &nchildren);
	if(children != NULL) {
		for(i = 0; i < nchildren; i++) {
			window_t	  w;
			XWindowAttributes xwa;

			if(XGetWindowAttributes(xdisplay, children[i], &xwa)) {
				if(xwa.override_redirect || xwa.map_state != IsViewable) continue;

				w.frame	 = wm_frame(xwa.width, xwa.height);
				w.client = children[i];

				XUnmapWindow(xdisplay, children[i]);

				save(w.client);

				arrput(windows, w);

				set_name(w.client);
			}
		}
		XFree(children);
	}

	while(1) {
		XNextEvent(xdisplay, &ev);
		if(ev.type == ButtonPress) {
			if(ev.type == ButtonPress) {
				for(i = 0; i < arrlen(windows); i++) {
					if(windows[i].frame->lowlevel->x11.window == ev.xbutton.window || parent_eq(ev.xbutton.window, windows[i].frame->lowlevel->x11.window)) {
						set_focus(windows[i].client);
						break;
					}
				}
				if(i != arrlen(windows)) continue;
				set_focus(ev.xbutton.window);
			}
		} else if(ev.type == FocusIn && ev.xfocus.window != focus) {
			set_focus(focus);
		} else if(ev.type == FocusOut && focus != None) {
			set_focus(focus);
		} else if(ev.type == ConfigureRequest) {
			XWindowChanges xwc;
			int	       i;
			Window	       w = ev.xconfigurerequest.window;
			for(i = 0; i < arrlen(windows); i++) {
				if(windows[i].client == w) {
					w = windows[i].frame->lowlevel->x11.window;
					break;
				}
			}

			xwc.x	   = ev.xconfigurerequest.x;
			xwc.y	   = ev.xconfigurerequest.y;
			xwc.width  = ev.xconfigurerequest.width;
			xwc.height = ev.xconfigurerequest.height;

			XConfigureWindow(xdisplay, w, CWX | CWY | CWWidth | CWHeight, &xwc);
		} else if(ev.type == ConfigureNotify) {
			int i;
			for(i = 0; i < arrlen(windows); i++) {
				if(windows[i].client == ev.xconfigure.window) {
					XWindowChanges	  xwc;
					XWindowAttributes xwa;
					xwc.x	   = ev.xconfigure.x;
					xwc.y	   = ev.xconfigure.y;
					xwc.width  = MwDefaultBorderWidth(windows[i].frame) * 2 + ev.xconfigure.width;
					xwc.height = MwDefaultBorderWidth(windows[i].frame) * 4 + TitleBarHeight + ev.xconfigure.height;

					XGetWindowAttributes(xdisplay, windows[i].frame->lowlevel->x11.window, &xwa);

					if(xwc.x != MwDefaultBorderWidth(windows[i].frame) || xwc.y != (MwDefaultBorderWidth(windows[i].frame) * 3 + TitleBarHeight)) {
						XConfigureWindow(xdisplay, windows[i].frame->lowlevel->x11.window, CWX | CWY | CWWidth | CWHeight, &xwc);

						xwc.x = MwDefaultBorderWidth(windows[i].frame);
						xwc.y = MwDefaultBorderWidth(windows[i].frame) * 3 + TitleBarHeight;
						XConfigureWindow(xdisplay, windows[i].client, CWX | CWY, &xwc);
					} else if(xwa.width != xwc.width || xwa.height != xwc.height) {
						XConfigureWindow(xdisplay, windows[i].frame->lowlevel->x11.window, CWWidth | CWHeight, &xwc);
					}
					break;
				}
			}
		} else if(ev.type == MapRequest) {
			window_t	  w;
			int		  i;
			int		  ret = 0;
			XWindowAttributes xwa;

			if(!XGetWindowAttributes(xdisplay, ev.xmaprequest.window, &xwa)) continue;

			for(i = 0; i < arrlen(windows); i++) {
				if(windows[i].frame->lowlevel->x11.window == ev.xmaprequest.window) break;
				if(windows[i].client == ev.xmaprequest.window) {
					/* what? */
					ret = 1;
					break;
				}
			}
			if(ret) continue;
			if(i != arrlen(windows)) {
				XWindowAttributes xwa;
				if(!XGetWindowAttributes(xdisplay, windows[i].client, &xwa)) {
					wm_destroy(windows[i].frame);
					arrdel(windows, i);
					continue;
				}

				XMoveWindow(xdisplay, windows[i].frame->lowlevel->x11.window, xwa.x, xwa.y);

				XMapWindow(xdisplay, ev.xmaprequest.window);
				pthread_mutex_lock(&xmutex);
				if(!XReparentWindow(xdisplay, windows[i].client, windows[i].frame->lowlevel->x11.window, MwDefaultBorderWidth(windows[i].frame), MwDefaultBorderWidth(windows[i].frame) * 3 + TitleBarHeight)) {
					pthread_mutex_unlock(&xmutex);
					wm_destroy(windows[i].frame);
					arrdel(windows, i);
					continue;
				}
				pthread_mutex_unlock(&xmutex);
				if(!XMapWindow(xdisplay, windows[i].client)) {
					wm_destroy(windows[i].frame);
					arrdel(windows, i);
					continue;
				}
				XFlush(xdisplay);
				continue;
			}

			w.frame	 = wm_frame(xwa.width, xwa.height);
			w.client = ev.xmaprequest.window;
			save(w.client);

			arrput(windows, w);

			set_name(w.client);
		} else if(ev.type == PropertyNotify) {
			int i;
			for(i = 0; i < arrlen(windows); i++) {
				if(windows[i].client == ev.xproperty.window) {
					break;
				}
			}
			if(arrlen(windows) == i) continue;

			if(ev.xproperty.atom == XInternAtom(xdisplay, "WM_NAME", False)) {
				set_name(windows[i].client);
			}
		} else if(ev.type == MapNotify) {
			int i;
			for(i = 0; i < arrlen(windows); i++) {
				if(windows[i].client == ev.xmap.window) {
					set_focus(windows[i].client);
					break;
				}
			}
		} else if(ev.type == UnmapNotify) {
			int i;
			for(i = 0; i < arrlen(windows); i++) {
				if(windows[i].client == ev.xunmap.window) {
					XWindowAttributes xwa;

					if(XGetWindowAttributes(xdisplay, windows[i].frame->lowlevel->x11.window, &xwa)) {
						pthread_mutex_lock(&xmutex);
						XReparentWindow(xdisplay, windows[i].client, DefaultRootWindow(xdisplay), xwa.x, xwa.y);
						pthread_mutex_unlock(&xmutex);
					}

					wm_destroy(windows[i].frame);
					arrdel(windows, i);
					break;
				}
			}

			if(focus == ev.xunmap.window) {
				Window w = None;
				int    i;

				for(i = arrlen(windows) - 1; i >= 0; i--) {
					XWindowAttributes xwa;

					XGetWindowAttributes(xdisplay, windows[i].client, &xwa);
					if(xwa.map_state == IsViewable) {
						w = windows[i].client;
					}
				}

				set_focus(w);
			}
		}
	}
	end_error();
}
