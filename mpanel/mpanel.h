#ifndef __MPANEL_H__
#define __MPANEL_H__

#include "config.h"

/* Milsko */
#include <MDE/Foundation.h>
#include <Mw/Milsko.h>

/* External */
#include <libconfig.h>

/* Standard */
#include <dlfcn.h>

/* main.c */
extern MwWidget window, box;

/* module.c */
void load_modules(void);

#endif
