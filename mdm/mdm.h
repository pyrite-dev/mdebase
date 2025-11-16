#ifndef __MDM_H__
#define __MDM_H__

#include "config.h"

#include <MDE/Foundation.h>

#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

/* xserver.c */
int launch_x(void);
int x_width(void);
int x_height(void);
void init_x(void);

/* login.c */
void login_window(void);

/* config.c */
extern char* config_picture;

void parse_config(void);

#endif
