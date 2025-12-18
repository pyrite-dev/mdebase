#include "config.h"

#include <stb_image.h>

#include <Mw/Milsko.h>
#include <libconfig.h>

#define TriW 5

void module(MwWidget box, config_setting_t* setting) {
	MwLLPixmap     closed = NULL;
	MwLLPixmap     opened = NULL;
	int	       w, h;
	unsigned int   ch;
	unsigned char* rgb;

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

	MwVaCreateWidget(MwButtonClass, "startbutton", box, 0, 0, 0, 0,
			 MwNfixedSize, MwGetInteger(box, MwNheight),
			 MwNflat, 1,
			 MwNpixmap, closed,
			 NULL);
}
