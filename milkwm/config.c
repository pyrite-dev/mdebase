#include "milkwm.h"

/* we prefix them for reasons */

xemil_t* wm_config = NULL;

void wm_config_init(void) {
}

static void read_config(const char* path) {
	xemil_t* tmp;

	if((tmp = xl_open_file(path)) == NULL || !xl_parse(tmp)) {
		fprintf(stderr, "MPanel config error: file %s\n", path);
		if(tmp != NULL) xl_close(tmp);
	} else {
		if(wm_config != NULL) xl_close(wm_config);

		wm_config = tmp;
	}
}

void wm_config_read(void) {
	char* conf = MDEDirectoryConfigPath();
	char* dir  = MwDirectoryJoin(conf, "milkwm");
	char* path = MwDirectoryJoin(dir, "milkwmrc");

	free(dir);
	free(conf);

	read_config(SYSCONFDIR "/milkwm/milkwmrc");
	read_config(path);

	free(path);
}
