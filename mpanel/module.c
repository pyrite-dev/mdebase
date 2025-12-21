#include "mpanel.h"

static void read_config(config_t* cfg, const char* path) {
	if(!config_read_file(cfg, path)) {
		fprintf(stderr, "MPanel config error: file %s at line %d: %s\n", path, config_error_line(cfg), config_error_text(cfg));
	}
}

void load_modules(void) {
	config_t	  cfg;
	config_setting_t* s;
	char*		  conf = MDEDirectoryConfigPath();
	char*		  dir  = MwDirectoryJoin(conf, "mpanel");
	char*		  path = MwDirectoryJoin(dir, "mpanelrc");

	free(dir);
	free(conf);

	config_init(&cfg);
	config_clear(&cfg);
	read_config(&cfg, SYSCONFDIR "/mpanel/mpanelrc");
	read_config(&cfg, path);

	free(path);

	if((s = config_lookup(&cfg, "Modules")) != NULL && (config_setting_type(s) == CONFIG_TYPE_LIST || config_setting_type(s) == CONFIG_TYPE_ARRAY)) {
		int		  i = 0;
		config_setting_t* c;
		while((c = config_setting_get_elem(s, i)) != NULL) {
			config_setting_t* type;
			if(config_setting_type(c) == CONFIG_TYPE_GROUP && (type = config_setting_lookup(c, "Type")) != NULL && config_setting_type(type) == CONFIG_TYPE_STRING) {
				char* p;
				void* lib;

				dir = MDEStringConcatenate(LIBDIR "/mpanel/lib", config_setting_get_string(type));
				p   = MDEStringConcatenate(dir, "Module.so");
				free(dir);

				if((lib = dlopen(p, RTLD_LOCAL | RTLD_LAZY)) != NULL) {
					void (*call)(MwWidget box, config_setting_t* setting) = dlsym(lib, "module");

					call(box, c);
				}

				free(p);
			}
			i++;
		}
	}

	config_destroy(&cfg);
}
