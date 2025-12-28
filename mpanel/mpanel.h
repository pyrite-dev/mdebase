#ifndef __MPANEL_H__
#define __MPANEL_H__

#include "config.h"

/* Milsko */
#include <MDE/Foundation.h>
#include <Mw/Milsko.h>

/* External */
#include <xemil.h>

/* Standard */
#include <dlfcn.h>
#include <unistd.h>
#include <signal.h>

/* main.c */
extern MwWidget window, box;

/* module.c */
void load_modules(void);
void reload_modules(void);

#endif
