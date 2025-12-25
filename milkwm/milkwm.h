#ifndef __MILKWM_H__
#define __MILKWM_H__

#include "config.h"

/* Milsko */
#include <MDE/Foundation.h>
#include <Mw/Milsko.h>

/* External */
#include <libconfig.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

/* Standard */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>

#define TitleBarHeight 18
#define Gap 1

/* xserver.c */
extern pthread_t       xthread;
extern pthread_mutex_t xmutex;
extern Display*	       xdisplay;

int  init_x(void);
void loop_x(void);
void set_focus_x(MwWidget widget);
void set_background_x(void);

/* wm.c */
extern MwWidget root;
extern int	wm_rehash;

void	 loop_wm(void);
MwWidget wm_frame(int w, int h);
void	 wm_destroy(MwWidget widget);
void	 wm_set_name(MwWidget widget, const char* name);
void	 wm_focus(MwWidget widget, int focus);
MwWidget wm_get_inside(MwWidget widget); /* make client area child of this widget, at (wm_content_x(),wm_content_y) */
int	 wm_entire_width(int content);
int	 wm_entire_height(int content);
int	 wm_content_width(int entire);
int	 wm_content_height(int entire);
int	 wm_content_x(void);
int	 wm_content_y(void);

/* config.c */
extern config_t wm_config;

void wm_config_init(void);
void wm_config_read(void);

#endif
