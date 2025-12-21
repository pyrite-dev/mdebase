#include "config.h"

#include <stb_image.h>
#include <stb_ds.h>

#define _MILSKO
#include <Mw/Milsko.h>
#include <libconfig.h>

#define TriW 5

typedef struct opaque {
	MwLLPixmap closed;
	MwLLPixmap opened;
	MwWidget   submenu;
	int	   is_opened;
	MwMenu	   menu;
} opaque_t;

static void menu(MwWidget handle, void* user, void* client) {
	opaque_t* opaque = handle->opaque;

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

void module(MwWidget box, config_setting_t* setting) {
	MwLLPixmap     closed = NULL;
	MwLLPixmap     opened = NULL;
	MwWidget       btn;
	int	       w, h;
	unsigned int   ch;
	unsigned char* rgb;
	opaque_t*      opaque = malloc(sizeof(*opaque));
	MwMenu	       m;

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

	int i;
	for(i = 0; i < 5; i++) {
		m	= malloc(sizeof(*m));
		m->name = MwStringDuplicate("Test");
		m->keep = 0;
		m->wsub = NULL;
		m->sub	= NULL;
		arrput(opaque->menu->sub, m);
	}

	btn->opaque = opaque;

	MwAddUserHandler(btn, MwNactivateHandler, activate, NULL);
	MwAddUserHandler(btn, MwNmenuHandler, menu, NULL);
}
