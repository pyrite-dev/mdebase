#ifndef __MILKWM_H__
#define __MILKWM_H__

#include "config.h"

/* Milsko */
#include <MDE/Foundation.h>
#include <Mw/Milsko.h>

/* External */
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

/* Standard */
#include <pthread.h>

#define TitleBarHeight 16

/* xserver.c */
extern pthread_t       xthread;
extern pthread_mutex_t xmutex;
extern Display*	       xdisplay;

int  init_x(void);
void loop_x(void);

/* wm.c */
void	 loop_wm(void);
MwWidget wm_frame(int w, int h);
void	 wm_set_name(MwWidget widget, const char* name);

#endif
