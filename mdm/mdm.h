#ifndef __MDM_H__
#define __MDM_H__

#include "config.h"

/* Milsko */
#include <MDE/Foundation.h>
#include <Mw/Milsko.h>

/* External */
#include <X11/Xlib.h>

/* Standard */
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <pthread.h>

/* login.c */
extern MwWidget root;
typedef enum session_type {
	MDM_SESSION_TYPE_X,
	MDM_SESSION_TYPE_WAYLAND,
} session_type_t;
typedef struct session_environment {
	char*	       run;
	char*	       try_run;
	session_type_t session_type;
	char*	       desktop_names;
} session_environment_t;

void login_window(void);

/* main.c */
extern gid_t		      gid;
extern uid_t		      uid;
extern session_environment_t* env;

/* xserver.c */
extern pthread_t       xthread;
extern pthread_mutex_t xmutex;
extern Display*	       xdisplay;

int  launch_x(void);
int  x_width(void);
int  x_height(void);
int  init_x(void);
void loop_x(void);
void kill_x(void);

/* config.c */
extern char* config_picture;

void parse_config(void);

#endif
