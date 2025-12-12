#define _MILSKO
#define USE_X11
#include "milkwm.h"

#include <stb_ds.h>

Display*	xdisplay;
pthread_t	xthread;
pthread_mutex_t xmutex;

static pthread_mutex_t focusmutex;

static Atom milkwm_set_focus;

typedef struct window {
	MwWidget frame;
	Window	 client;
	int	 working;
} window_t;
static window_t* windows = NULL;

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
	while(1) {
		pthread_mutex_lock(&xmutex);
		if(root != NULL) {
			pthread_mutex_unlock(&xmutex);
			break;
		}
		pthread_mutex_unlock(&xmutex);
	}

	loop_x();
	XCloseDisplay(xdisplay);
	return NULL;
}

static Window focus = None, nofocus;
static void   set_focus(Window w) {
	  int i;
	  for(i = 0; i < arrlen(windows); i++) {
		  pthread_mutex_lock(&focusmutex);
		  if(windows[i].client == w) {
			  pthread_mutex_unlock(&focusmutex);
			  wm_focus(windows[i].frame, 1);
		  } else if(focus != w && focus == windows[i].client) {
			  pthread_mutex_unlock(&focusmutex);
			  wm_focus(windows[i].frame, 0);
		  } else {
			  pthread_mutex_unlock(&focusmutex);
		  }
	  }

	  pthread_mutex_lock(&focusmutex);
	  focus = w;
	  if(w == None) {
		  XSetInputFocus(xdisplay, nofocus, RevertToParent, CurrentTime);
	  } else {
		  XSetInputFocus(xdisplay, w, RevertToParent, CurrentTime);
	  }
	  pthread_mutex_unlock(&focusmutex);
}

int init_x(void) {
	void*		     old;
	XSetWindowAttributes xswa;
	Cursor		     cur;

	xswa.override_redirect = True;

	xdisplay = XOpenDisplay(NULL);

	begin_error();
	XSelectInput(xdisplay, DefaultRootWindow(xdisplay), SubstructureRedirectMask | SubstructureNotifyMask);
	end_error();

	if(error_happened) return 1;

	nofocus = XCreateSimpleWindow(xdisplay, DefaultRootWindow(xdisplay), -10, -10, 1, 1, 0, 0, 0);
	XChangeWindowAttributes(xdisplay, nofocus, CWOverrideRedirect, &xswa);
	XMapWindow(xdisplay, nofocus);
	set_focus(None);

	XChangeProperty(xdisplay, DefaultRootWindow(xdisplay), XInternAtom(xdisplay, "_NET_SUPPORTING_WM_CHECK", False), XA_WINDOW, 32, PropModeReplace, (unsigned char*)&nofocus, 1);
	XChangeProperty(xdisplay, nofocus, XInternAtom(xdisplay, "_NET_SUPPORTING_WM_CHECK", False), XA_WINDOW, 32, PropModeReplace, (unsigned char*)&nofocus, 1);
	XChangeProperty(xdisplay, nofocus, XInternAtom(xdisplay, "_NET_WM_NAME", False), XInternAtom(xdisplay, "UTF8_STRING", False), 8, PropModeReplace, (unsigned char*)"milkwm", 6);

	cur = MwLLX11CreateCursor(xdisplay, &MwCursorDefault, &MwCursorDefaultMask);
	XDefineCursor(xdisplay, DefaultRootWindow(xdisplay), cur);
	XFreeCursor(xdisplay, cur);

	milkwm_set_focus = XInternAtom(xdisplay, "MILKWM_SET_FOCUS", False);

	pthread_mutex_init(&xmutex, NULL);
	pthread_mutex_init(&focusmutex, NULL);

	pthread_create(&xthread, NULL, x11_thread_routine, NULL);

	return 0;
}

static void save(Window w) {
	XGrabButton(xdisplay, 1, 0, w, True, ButtonPressMask | ButtonReleaseMask | PointerMotionMask, GrabModeSync, GrabModeAsync, None, None);
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
	unsigned char* buf  = NULL;
	Atom	       atom = XInternAtom(xdisplay, "WM_NAME", False);
	int	       i;

	for(i = 0; i < arrlen(windows); i++) {
		if(windows[i].client == wnd) {
			if(XGetWindowProperty(xdisplay, wnd, atom, 0, 1024, False, XA_STRING, &type, &format, &nitem, &after, &buf) == Success && buf != NULL) {
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
		if((ev.type == ButtonPress || ev.type == ButtonRelease) && ev.xbutton.button == Button1) {
			Window w = None, t = ev.xbutton.window;
			if(ev.xbutton.subwindow != None) t = ev.xbutton.subwindow;
			if(ev.type == ButtonPress) {
				for(i = 0; i < arrlen(windows); i++) {
					if(windows[i].frame->lowlevel->x11.window == t || parent_eq(t, windows[i].frame->lowlevel->x11.window)) {
						set_focus(windows[i].client);
						w = windows[i].frame->lowlevel->x11.window;
						break;
					}
				}
			}

			if(w != None) XRaiseWindow(xdisplay, w);

			XAllowEvents(xdisplay, ReplayPointer, CurrentTime);
		} else if(ev.type == MotionNotify && ev.xmotion.subwindow != None) {
			Window w = None, t = ev.xmotion.window;
			if(ev.xmotion.subwindow != None) t = ev.xmotion.subwindow;
			for(i = 0; i < arrlen(windows); i++) {
				if(windows[i].frame->lowlevel->x11.window == t || parent_eq(t, windows[i].frame->lowlevel->x11.window)) {
					w = windows[i].frame->lowlevel->x11.window;
					break;
				}
			}

			if(w != None) XRaiseWindow(xdisplay, w);

			XAllowEvents(xdisplay, ReplayPointer, CurrentTime);
		} else if(ev.type == FocusIn && ev.xfocus.window != focus) {
			set_focus(focus);
		} else if(ev.type == FocusOut && focus != None) {
			set_focus(focus);
		} else if(ev.type == ConfigureRequest) {
			XWindowChanges xwc;
			int	       i;
			Window	       w = ev.xconfigurerequest.window;
			for(i = 0; i < arrlen(windows); i++) {
				if(windows[i].frame->lowlevel->x11.window == w) {
					xwc.x	   = wm_content_x();
					xwc.y	   = wm_content_y();
					xwc.width  = wm_content_width(ev.xconfigurerequest.width);
					xwc.height = wm_content_height(ev.xconfigurerequest.height);

					XConfigureWindow(xdisplay, windows[i].client, CWX | CWY | CWWidth | CWHeight, &xwc);
					break;
				}
			}

			xwc.x	   = ev.xconfigurerequest.x;
			xwc.y	   = ev.xconfigurerequest.y;
			xwc.width  = ev.xconfigurerequest.width;
			xwc.height = ev.xconfigurerequest.height;

			XConfigureWindow(xdisplay, w, ev.xconfigurerequest.value_mask, &xwc);
		} else if(ev.type == ConfigureNotify) {
			int i;
			for(i = 0; i < arrlen(windows); i++) {
				if(windows[i].client == ev.xconfigure.window) {
					XWindowChanges	  xwc;
					XWindowAttributes xwa;

					xwc.x	   = ev.xconfigure.x;
					xwc.y	   = ev.xconfigure.y;
					xwc.width  = wm_entire_width(ev.xconfigure.width);
					xwc.height = wm_entire_height(ev.xconfigure.height);

					XGetWindowAttributes(xdisplay, windows[i].frame->lowlevel->x11.window, &xwa);

					if(xwc.x != wm_content_x() || xwc.y != wm_content_y()) {
						XConfigureWindow(xdisplay, windows[i].frame->lowlevel->x11.window, CWX | CWY | CWWidth | CWHeight, &xwc);

						xwc.x = wm_content_x();
						xwc.y = wm_content_y();
						XConfigureWindow(xdisplay, windows[i].client, CWX | CWY, &xwc);
					} else if(xwa.width != xwc.width || xwa.height != xwc.height) {
						XConfigureWindow(xdisplay, windows[i].frame->lowlevel->x11.window, CWWidth | CWHeight, &xwc);
					}

					break;
				}
			}
		} else if(ev.type == CreateNotify) {
			XWindowAttributes xwa;

			XGetWindowAttributes(xdisplay, DefaultRootWindow(xdisplay), &xwa);

			XMoveWindow(xdisplay, ev.xcreatewindow.window, rand() % (xwa.width - wm_entire_width(ev.xcreatewindow.width)), rand() % (xwa.height - wm_entire_height(ev.xcreatewindow.height)));
		} else if(ev.type == DestroyNotify) {
			for(i = 0; i < arrlen(windows); i++) {
				if(windows[i].client == ev.xdestroywindow.window) {
					wm_destroy(windows[i].frame);
					arrdel(windows, i);
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

				XMoveWindow(xdisplay, ev.xmaprequest.window, xwa.x, xwa.y);
				XMapWindow(xdisplay, ev.xmaprequest.window);
				do {
					XFlush(xdisplay);
					XGetWindowAttributes(xdisplay, ev.xmaprequest.window, &xwa);
				} while(xwa.map_state != IsViewable);

				if(!XReparentWindow(xdisplay, windows[i].client, wm_get_inside(windows[i].frame)->lowlevel->x11.window, wm_content_x(), wm_content_y())) {
					wm_destroy(windows[i].frame);
					arrdel(windows, i);
					continue;
				}
				XMapWindow(xdisplay, windows[i].client);
				continue;
			}

			w.frame	  = wm_frame(xwa.width, xwa.height);
			w.client  = ev.xmaprequest.window;
			w.working = 1;
			save(w.client);

			if(ret) {
				wm_destroy(w.frame);
			} else {
				arrput(windows, w);

				set_name(w.client);
			}
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
			if(ev.xmap.override_redirect) continue;

			for(i = 0; i < arrlen(windows); i++) {
				if(windows[i].client == ev.xmap.window) {
					if(windows[i].frame == NULL) printf("wtf?\n");
					set_focus(windows[i].client);
					windows[i].working = 0;
					break;
				} else if(windows[i].frame->lowlevel->x11.window == ev.xmap.window || nofocus == ev.xmap.window)
					break;
			}

			if(i == arrlen(windows)) {
				window_t	  w;
				XWindowAttributes xwa;

				XGetWindowAttributes(xdisplay, ev.xmap.window, &xwa);

				w.frame	  = wm_frame(xwa.width, xwa.height);
				w.client  = ev.xmap.window;
				w.working = 1;
				save(w.client);
				arrput(windows, w);
			}
		} else if(ev.type == UnmapNotify) {
			int i;
			int nvm = 0;
			for(i = 0; i < arrlen(windows); i++) {
				if(windows[i].client == ev.xunmap.window) {
					XWindowAttributes xwa;
					int		  x, y;

					if(XGetWindowAttributes(xdisplay, windows[i].client, &xwa)) {
						if(xwa.map_state == IsViewable) {
							nvm = 1;
							break;
						}
					} else if(windows[i].working) {
						nvm = 1;
						break;
					}

					x = xwa.x;
					y = xwa.y;
					if(XGetWindowAttributes(xdisplay, windows[i].frame->lowlevel->x11.window, &xwa)) {
						XReparentWindow(xdisplay, windows[i].client, DefaultRootWindow(xdisplay), xwa.x + x, xwa.y + y);
					}

					wm_destroy(windows[i].frame);
					arrdel(windows, i);
					break;
				}
			}
			if(nvm) continue;

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
		} else if(ev.type == ClientMessage) {
			if(ev.xclient.message_type == milkwm_set_focus && ev.xclient.data.l[0] == milkwm_set_focus) {
				set_focus(ev.xclient.data.l[1]);
			}
		}
	}
	end_error();
}

void set_focus_x(MwWidget widget) {
	int i;

	for(i = 0; i < arrlen(windows); i++) {
		if(windows[i].frame == widget) {
			XEvent ev;

			ev.type			= ClientMessage;
			ev.xclient.window	= nofocus;
			ev.xclient.message_type = milkwm_set_focus;
			ev.xclient.format	= 32;
			ev.xclient.data.l[0]	= ev.xclient.message_type;
			ev.xclient.data.l[1]	= windows[i].client;

			XSendEvent(xdisplay, nofocus, False, 0, &ev);

			XRaiseWindow(xdisplay, windows[i].frame->lowlevel->x11.window);

			break;
		}
	}
}
