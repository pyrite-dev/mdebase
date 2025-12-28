#include "config.h"

#include <stb_image.h>
#include <stb_ds.h>

#define _MILSKO
#include <MDE/Foundation.h>
#include <Mw/Milsko.h>
#include <xemil.h>
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
	MwMenu	   current;

	MwLLPixmap stripe;
	char*	   stripe_color;

	char* name;
	char* category;
	char* exec;
} opaque_t;

static int dumper(void* user, const char* section, const char* name, const char* value) {
	opaque_t* opaque = user;
	if(strcmp(section, "Desktop Entry") != 0) return 1;

	if(strcmp(name, "Name") == 0) {
		opaque->name = MDEStringDuplicate(value);
	} else if(strcmp(name, "Categories") == 0) {
		opaque->category = MDEStringDuplicate(value);
	} else if(strcmp(name, "Exec") == 0) {
		opaque->exec = MDEStringDuplicate(value);
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
		for(i = 0; i < arrlen(opaque->current->sub); i++) {
			if(strcmp(opaque->current->sub[i]->name, category_mapping[c]) == 0) {
				mc = opaque->current->sub[i];
				break;
			}
		}
		if(mc == NULL) {
			mc	   = malloc(sizeof(*mc));
			mc->name   = MDEStringDuplicate(category_mapping[c]);
			mc->keep   = 0;
			mc->sub	   = NULL;
			mc->wsub   = NULL;
			mc->opaque = NULL;

			arrput(opaque->current->sub, mc);
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
		MwPoint	 p;
		MwWidget fill, image;
		MwRect	 rc;

		MwVaApply(handle,
			  MwNpixmap, opaque->opened,
			  MwNforceInverted, 1,
			  NULL);

		opaque->submenu = MwVaCreateWidget(MwSubMenuClass, "submenu", handle, 0, MwGetInteger(handle, MwNheight), 0, 0,
						   MwNleftPadding, 16 + MwDefaultBorderWidth(handle),
						   NULL);

		MwSubMenuGetSize(opaque->submenu, opaque->menu, &rc);

		fill = MwVaCreateWidget(MwFrameClass, "fill", opaque->submenu, MwDefaultBorderWidth(handle), MwDefaultBorderWidth(handle), 16, rc.height - MwDefaultBorderWidth(handle) * 2,
					MwNbackground, opaque->stripe_color,
					NULL);

		image = MwVaCreateWidget(MwImageClass, "image", fill, 0, MwGetInteger(fill, MwNheight) - 96, 16, 96,
					 MwNpixmap, opaque->stripe,
					 NULL);

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

static int sort_alphabet(const void* a, const void* b) {
	return strcmp((*((MwMenu*)a))->name, (*((MwMenu*)b))->name);
}

static char* node_get_name(xl_node_t* node) {
	xl_attribute_t* a = node->first_attribute;

	while(a != NULL) {
		if(strcmp(a->key, "Name") == 0) return a->value;

		a = a->next;
	}

	return NULL;
}

static void menu_scan(opaque_t* opaque, xl_node_t* node) {
	xl_node_t* n = node->first_child;

	while(n != NULL) {
		char*  name = node_get_name(n);
		MwMenu m    = NULL;

		if(n->type == XL_NODE_NODE && strcmp(n->name, "Application") == 0 && name != NULL) {
			m	  = malloc(sizeof(*m));
			m->name	  = MDEStringDuplicate(name);
			m->keep	  = 0;
			m->wsub	  = NULL;
			m->sub	  = NULL;
			m->opaque = NULL;

			opaque->current = m;

			MDEDirectoryScan(DATAROOTDIR "/applications", call, opaque);
#if defined(__NetBSD__)
			MDEDirectoryScan("/usr/pkg/share/applications", call, opaque);
#endif
			MDEDirectoryScan("/usr/share/applications", call, opaque);
		} else if(n->type == XL_NODE_NODE && strcmp(n->name, "Separator") == 0) {
			m	  = malloc(sizeof(*m));
			m->name	  = MDEStringDuplicate("----");
			m->keep	  = 0;
			m->wsub	  = NULL;
			m->sub	  = NULL;
			m->opaque = NULL;
		} else if(n->type == XL_NODE_NODE && strcmp(n->name, "Execute") == 0 && name != NULL) {
			m	  = malloc(sizeof(*m));
			m->name	  = MDEStringDuplicate(name);
			m->keep	  = 0;
			m->wsub	  = NULL;
			m->sub	  = NULL;
			m->opaque = MDEStringDuplicate(n->text == NULL ? "" : n->text);
		} else if(n->type == XL_NODE_NODE && strcmp(n->name, "EndSession") == 0 && name != NULL) {
			m	  = malloc(sizeof(*m));
			m->name	  = MDEStringDuplicate(name);
			m->keep	  = 0;
			m->wsub	  = NULL;
			m->sub	  = NULL;
			m->opaque = malloc(512);

			sprintf(m->opaque, "kill %ld", (long)getppid());
		} else if(n->type == XL_NODE_NODE && strcmp(n->name, "ShutDown") == 0 && name != NULL) {
			m	  = malloc(sizeof(*m));
			m->name	  = MDEStringDuplicate(name);
			m->keep	  = 0;
			m->wsub	  = NULL;
			m->sub	  = NULL;
			m->opaque = MDEStringDuplicate(""); /* TODO */
		} else if(n->type == XL_NODE_NODE && strcmp(n->name, "Reboot") == 0 && name != NULL) {
			m	  = malloc(sizeof(*m));
			m->name	  = MDEStringDuplicate(name);
			m->keep	  = 0;
			m->wsub	  = NULL;
			m->sub	  = NULL;
			m->opaque = MDEStringDuplicate(""); /* TODO */
		}

		if(m != NULL) {
			arrput(opaque->menu->sub, m);
		}

		n = n->next;
	}
}

static void recursive_free(MwMenu menu) {
	int i;

	for(i = 0; i < arrlen(menu->sub); i++) recursive_free(menu->sub[i]);
	if(menu->opaque != NULL) free(menu->opaque);
	if(menu->name != NULL) free(menu->name);
	if(menu->wsub != NULL) MwDestroyWidget(menu->wsub);
	free(menu);
}

void module_reload(MwWidget box, MwWidget user, xl_node_t* node) {
	xl_node_t* n;
	opaque_t*  opaque = user->opaque;
	int	   i;

	if(opaque->menu != NULL) recursive_free(opaque->menu);

	opaque->menu	     = malloc(sizeof(*opaque->menu));
	opaque->menu->name   = NULL;
	opaque->menu->keep   = 0;
	opaque->menu->wsub   = NULL;
	opaque->menu->sub    = NULL;
	opaque->menu->opaque = NULL;

	n = node->first_child;

	while(n != NULL) {
		if(n->type == XL_NODE_NODE && strcmp(n->name, "Menu") == 0) {
			menu_scan(opaque, n);
		} else if(n->type == XL_NODE_NODE && strcmp(n->name, "Stripe") == 0) {
			xl_attribute_t* a;

			if(opaque->stripe != NULL) MwLLDestroyPixmap(opaque->stripe);
			opaque->stripe = MwLoadImage(box, n->text == NULL ? "" : n->text);

			a = n->first_attribute;
			while(a != NULL) {
				if(strcmp(a->key, "Color") == 0 && a->value != NULL) {
					if(opaque->stripe_color != NULL) free(opaque->stripe_color);

					opaque->stripe_color = MDEStringDuplicate(a->value);
				}

				a = a->next;
			}
		}

		n = n->next;
	}

	if(arrlen(opaque->menu->sub[0]->sub) > 0) {
		qsort(opaque->menu->sub[0]->sub, arrlen(opaque->menu->sub[0]->sub), sizeof(MwMenu), sort_alphabet);
		for(i = 0; i < arrlen(opaque->menu->sub[0]->sub); i++) {
			qsort(opaque->menu->sub[0]->sub[i]->sub, arrlen(opaque->menu->sub[0]->sub[i]->sub), sizeof(MwMenu), sort_alphabet);
		}
	}
}

MwWidget module(MwWidget box, xl_node_t* node) {
	MwLLPixmap     closed = NULL;
	MwLLPixmap     opened = NULL;
	MwWidget       btn;
	int	       w, h;
	unsigned int   ch;
	unsigned char* rgb;
	opaque_t*      opaque = malloc(sizeof(*opaque));

	if((rgb = stbi_load(ICON64DIR "/apps/mde.png", &w, &h, &ch, 4)) != NULL) {
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

	btn->opaque = opaque;

	opaque->closed	  = closed;
	opaque->opened	  = opened;
	opaque->submenu	  = NULL;
	opaque->is_opened = 0;

	opaque->stripe_color = NULL;

	opaque->stripe = NULL;

	opaque->menu = NULL;

	module_reload(box, btn, node);

	MwAddUserHandler(btn, MwNactivateHandler, activate, NULL);
	MwAddUserHandler(btn, MwNmenuHandler, menu, NULL);

	return btn;
}
