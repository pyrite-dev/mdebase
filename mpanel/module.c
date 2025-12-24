#include "mpanel.h"

static void read_config(xemil_t** cfg, const char* path) {
	xemil_t* tmp;

	if((tmp = xl_open_file(path)) == NULL || !xl_parse(tmp)) {
		fprintf(stderr, "MPanel config error: file %s\n", path);
		if(tmp != NULL) xl_close(tmp);
	} else {
		if((*cfg) != NULL) xl_close(*cfg);

		*cfg = tmp;
	}
}

static void module_scan(xl_node_t* node) {
	xl_node_t* n = node->first_child;

	while(n != NULL) {
		if(n->type == XL_NODE_NODE) {
			char* p	  = MDEStringConcatenate("lib", n->name);
			char* p2  = MDEStringConcatenate(p, "Module.so");
			char* s	  = MwDirectoryJoin(LIBDIR "/mpanel", p2);
			void* lib = dlopen(s, RTLD_LAZY | RTLD_LOCAL);

			if(lib != NULL) {
				void (*module)(MwWidget box, xl_node_t* node) = dlsym(lib, "module");

				if(module != NULL) module(box, n);
			}

			free(s);
			free(p2);
			free(p);
		}

		n = n->next;
	}
}

void load_modules(void) {
	xemil_t* cfg  = NULL;
	char*	 conf = MDEDirectoryConfigPath();
	char*	 dir  = MwDirectoryJoin(conf, "mpanel");
	char*	 path = MwDirectoryJoin(dir, "mpanelrc");

	free(dir);
	free(conf);

	read_config(&cfg, SYSCONFDIR "/mpanel/mpanelrc");
	read_config(&cfg, path);

	free(path);

	if(cfg != NULL && cfg->root != NULL) {
		xl_node_t* n = cfg->root->first_child;
		if(strcmp(cfg->root->name, "Panel") != 0) {
			fprintf(stderr, "MPanel config error: Root element has to be Panel\n");
			return;
		}

		while(n != NULL) {
			if(n->type == XL_NODE_NODE && strcmp(n->name, "Module") == 0) {
				module_scan(n);
			}

			n = n->next;
		}

		xl_close(cfg);
	}
}
