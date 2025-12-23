#include "config.h"

#include <stb_image.h>
#include <stb_ds.h>

#define _MILSKO
#include <MDE/Foundation.h>
#include <Mw/Milsko.h>
#include <libconfig.h>
#include <ini.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define TriW 5

typedef struct opaque {
	MwLLPixmap closed;
	MwLLPixmap opened;
	MwWidget   submenu;
	int	   is_opened;
	MwMenu	   menu;

	char* name;
	char* category;
	char* exec;
} opaque_t;

static int dumper(void* user, const char* section, const char* name, const char* value) {
	opaque_t* opaque = user;
	if(strcmp(section, "Desktop Entry") != 0) return 1;

	if(strcmp(name, "Name") == 0) {
		opaque->name = MwStringDuplicate(value);
	} else if(strcmp(name, "Categories") == 0) {
		opaque->category = MwStringDuplicate(value);
	} else if(strcmp(name, "Exec") == 0) {
		opaque->exec = MwStringDuplicate(value);
	}

	return 1;
}

enum CATEGORY {
	C_UNKNOWN = 0,
	C_AUDIOVIDEO,
	C_AUDIO,
	C_VIDEO,
	C_DEVELOPMENT,
	C_EDUCATION,
	C_GAME,
	C_GRAPHICS,
	C_NETWORK,
	C_OFFICE,
	C_SCIENCE,
	C_SETTINGS,
	C_SYSTEM,
	C_UTILITY
};

static const char* category_mapping[] = {
    "Lost & Found",
    "Multimedia",
    "Audio",
    "Video",
    "Development",
    "Education",
    "Game",
    "Graphics",
    "Network",
    "Office",
    "Science",
    "Settings",
    "System",
    "Utility"};

static void call(const char* name, int dir, int symlink, void* user) {
	opaque_t* opaque = user;
	int	  c	 = C_UNKNOWN;

	opaque->name	 = NULL;
	opaque->category = NULL;
	opaque->exec	 = NULL;
	ini_parse(name, dumper, user);

	if(opaque->category != NULL) {
		char* str = opaque->category;

		while(str != NULL) {
			char* s = strchr(str, ';');

			if(s != NULL) {
				s[0] = 0;
			}

			if(strcasecmp(str, "AudioVideo") == 0) {
				c = C_AUDIOVIDEO;
			} else if(strcasecmp(str, "Audio") == 0) {
				c = C_AUDIO;
			} else if(strcasecmp(str, "Video") == 0) {
				c = C_VIDEO;
			} else if(strcasecmp(str, "Development") == 0) {
				c = C_DEVELOPMENT;
			} else if(strcasecmp(str, "Education") == 0) {
				c = C_EDUCATION;
			} else if(strcasecmp(str, "Game") == 0) {
				c = C_GAME;
			} else if(strcasecmp(str, "Graphics") == 0) {
				c = C_GRAPHICS;
			} else if(strcasecmp(str, "Network") == 0) {
				c = C_NETWORK;
			} else if(strcasecmp(str, "Office") == 0) {
				c = C_OFFICE;
			} else if(strcasecmp(str, "Science") == 0) {
				c = C_SCIENCE;
			} else if(strcasecmp(str, "Settings") == 0) {
				c = C_SETTINGS;
			} else if(strcasecmp(str, "System") == 0) {
				c = C_SYSTEM;
			} else if(strcasecmp(str, "Utility") == 0) {
				c = C_UTILITY;
			}

			str = s;
			if(str != NULL) str++;
		}

		free(opaque->category);
	}

	if(opaque->name != NULL) {
		int    i;
		MwMenu mc = NULL, m;
		for(i = 0; i < arrlen(opaque->menu->sub); i++) {
			if(strcmp(opaque->menu->sub[i]->name, category_mapping[c]) == 0) {
				mc = opaque->menu->sub[i];
				break;
			}
		}
		if(mc == NULL) {
			mc	 = malloc(sizeof(*mc));
			mc->name = (char*)category_mapping[c];
			mc->keep = 0;
			mc->sub	 = NULL;
			mc->wsub = NULL;

			arrput(opaque->menu->sub, mc);
		}

		for(i = 0; i < arrlen(mc->sub); i++) {
			if(strcmp(mc->sub[i]->name, opaque->name) == 0) {
				break;
			}
		}

		if(i == arrlen(mc->sub)) {
			m	  = malloc(sizeof(*m));
			m->name	  = opaque->name;
			m->keep	  = 0;
			m->sub	  = NULL;
			m->wsub	  = NULL;
			m->opaque = opaque->exec;

			arrput(mc->sub, m);
		}
	}
}

static char** desktop_exec(const char* exec, const char* file) {
	char** a = NULL;
	int    i;
	char*  buf = malloc(1);

	buf[0] = 0;

	for(i = 0;; i++) {
		if(exec[i] == ' ' || exec[i] == 0) {
			if(strlen(buf) > 0) arrput(a, buf);

			buf    = malloc(1);
			buf[0] = 0;

			if(exec[i] == 0) break;
		} else if(exec[i] == '%') {
			char c = exec[++i];

			if(c == 'f' && file != NULL) {
				char* old = buf;

				buf = malloc(strlen(old) + strlen(file) + 1);
				strcpy(buf, old);
				strcpy(buf + strlen(old), file);

				free(old);
			}
		} else {
			char* old = buf;

			buf = malloc(strlen(old) + 2);
			strcpy(buf, old);
			buf[strlen(old)]     = exec[i];
			buf[strlen(old) + 1] = 0;

			free(old);
		}
	}

	arrput(a, NULL);

	return a;
}

static void menu(MwWidget handle, void* user, void* client) {
	opaque_t* opaque = handle->opaque;
	char*	  s	 = NULL;
	MwMenu	  m	 = client;

	if(m->opaque != NULL) {
		char** args = desktop_exec(m->opaque, NULL);
		int    i;

		if(fork() == 0) {
			execvp(args[0], args);

			_exit(-1);
		}

		for(i = 0; i < arrlen(args); i++) {
			if(args[i] != NULL) free(args[i]);
		}
		arrfree(args);
	}

	if(opaque->is_opened) {
		opaque->is_opened = 0;

		opaque->submenu = NULL;

		MwVaApply(handle,
			  MwNpixmap, opaque->closed,
			  MwNforceInverted, 0,
			  NULL);
	}
}

static void activate(MwWidget handle, void* user, void* client) {
	opaque_t* opaque = handle->opaque;

	opaque->is_opened = opaque->is_opened ? 0 : 1;
	if(opaque->is_opened) {
		MwPoint p;

		MwVaApply(handle,
			  MwNpixmap, opaque->opened,
			  MwNforceInverted, 1,
			  NULL);

		opaque->submenu = MwCreateWidget(MwSubMenuClass, "submenu", handle, 0, MwGetInteger(handle, MwNheight), 0, 0);

		p.x = 0;
		p.y = 0;
		MwSubMenuAppear(opaque->submenu, opaque->menu, &p, 1);
	} else {
		MwVaApply(handle,
			  MwNpixmap, opaque->closed,
			  MwNforceInverted, 0,
			  NULL);

		if(opaque->submenu != NULL) MwDestroyWidget(opaque->submenu);
	}
}

int sort_alphabet(const void* a, const void* b) {
	return strcmp((*((MwMenu*)a))->name, (*((MwMenu*)b))->name);
}

void module(MwWidget box, config_setting_t* setting) {
	MwLLPixmap     closed = NULL;
	MwLLPixmap     opened = NULL;
	MwWidget       btn;
	int	       w, h;
	unsigned int   ch;
	unsigned char* rgb;
	opaque_t*      opaque = malloc(sizeof(*opaque));
	MwMenu	       m;
	int	       i;

	if((rgb = stbi_load(ICON64DIR "/logo.png", &w, &h, &ch, 4)) != NULL) {
		int	       bw   = MwGetInteger(box, MwNheight) - MwDefaultBorderWidth(box) * 2;
		unsigned char* data = malloc(bw * bw * 4);
		unsigned char* save = malloc(bw * bw * 4);
		int	       y, x;
		int	       max = (TriW - 1) / 2;

		memset(data, 0, bw * bw * 4);
		for(y = 0; y < h; y++) {
			int fy = y * bw / h;
			for(x = 0; x < w; x++) {
				int fx = x * bw / w;
				memcpy(&data[(fy * bw + fx) * 4], &rgb[(y * w + x) * 4], 4);
			}
		}
		memcpy(save, data, bw * bw * 4);

		for(y = 0; y < max + 1; y++) {
			int tw = y * 2 + 1;
			int ix = w - TriW + (max - y);

			for(x = 0; x < tw; x++) {
				data[4 * (y * bw + x + ix) + 3] = 255;
			}
		}
		closed = MwLoadRaw(box, data, bw, bw);

		memcpy(data, save, bw * bw * 4);

		max = (TriW - 1) / 2;
		for(y = 0; y < max + 1; y++) {
			int tw = (max - y) * 2 + 1;
			int ix = w - TriW + y;

			for(x = 0; x < tw; x++) {
				data[4 * (y * bw + x + ix) + 3] = 255;
			}
		}
		opened = MwLoadRaw(box, data, bw, bw);

		free(save);
		free(data);
		free(rgb);
	}

	btn = MwVaCreateWidget(MwButtonClass, "startbutton", box, 0, 0, 0, 0,
			       MwNfixedSize, MwGetInteger(box, MwNheight),
			       MwNflat, 1,
			       MwNpixmap, closed,
			       NULL);

	opaque->closed	  = closed;
	opaque->opened	  = opened;
	opaque->submenu	  = NULL;
	opaque->is_opened = 0;

	opaque->menu	   = malloc(sizeof(*opaque->menu));
	opaque->menu->name = NULL;
	opaque->menu->keep = 0;
	opaque->menu->wsub = NULL;
	opaque->menu->sub  = NULL;

	MDEDirectoryScan(DATAROOTDIR "/applications", call, opaque);
	MDEDirectoryScan("/usr/share/applications", call, opaque);
#if defined(__NetBSD__)
	MDEDirectoryScan("/usr/pkg/share/applications", call, opaque);
#endif
	qsort(opaque->menu->sub, arrlen(opaque->menu->sub), sizeof(MwMenu), sort_alphabet);

	for(i = 0; i < arrlen(opaque->menu->sub); i++) {
		qsort(opaque->menu->sub[i]->sub, arrlen(opaque->menu->sub[i]->sub), sizeof(MwMenu), sort_alphabet);
	}

	btn->opaque = opaque;

	MwAddUserHandler(btn, MwNactivateHandler, activate, NULL);
	MwAddUserHandler(btn, MwNmenuHandler, menu, NULL);
}
