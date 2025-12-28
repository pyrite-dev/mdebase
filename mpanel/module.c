#include "mpanel.h"

#include <stb_ds.h>

typedef struct module {
	char*	 key;
	void*	 value;
	MwWidget user;
} module_t;
module_t* modules = NULL;

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

static void init_module(xl_node_t* node) {
	char*		p  = MDEStringConcatenate("lib", node->name);
	char*		p2 = MDEStringConcatenate(p, "Module.so");
	char*		s  = MwDirectoryJoin(LIBDIR "/mpanel", p2);
	void*		lib;
	char*		id = NULL;
	xl_attribute_t* a  = node->first_attribute;
	int		ind;

	while(a != NULL) {
		if(strcmp(a->key, "ID") == 0 && a->value != NULL) {
			id = a->value;
		}
		a = a->next;
	}

	if(id == NULL) return;

	lib = dlopen(s, RTLD_LAZY | RTLD_LOCAL);

	if(lib != NULL) {
		if((ind = shgeti(modules, id)) == -1) {
			MwWidget (*module)(MwWidget box, xl_node_t* node) = dlsym(lib, "module");
			void* ret					  = NULL;

			if(module != NULL) {
				ret = module(box, node);

				shput(modules, id, lib);

				ind		  = shgeti(modules, id);
				modules[ind].user = ret;
			}
		} else {
			void (*module)(MwWidget box, MwWidget user, xl_node_t* node);
			void* ret = NULL;

			dlclose(lib);
			lib = modules[ind].value;

			module = dlsym(lib, "module_reload");

			if(module != NULL) {
				module(box, modules[ind].user, node);
			}
		}
	}

	free(s);
	free(p2);
	free(p);
}

static void module_scan(xl_node_t* node) {
	xl_node_t* n = node->first_child;

	while(n != NULL) {
		if(n->type == XL_NODE_NODE) {
			init_module(n);
		}

		n = n->next;
	}
}

static void prepare_config(xemil_t** cfg) {
	char* conf = MDEDirectoryConfigPath();
	char* dir  = MwDirectoryJoin(conf, "mpanel");
	char* path = MwDirectoryJoin(dir, "mpanelrc");

	free(dir);
	free(conf);

	read_config(cfg, SYSCONFDIR "/mpanel/mpanelrc");
	read_config(cfg, path);

	free(path);
}

void load_modules(void) {
	xemil_t* cfg = NULL;

	prepare_config(&cfg);

	sh_new_strdup(modules);

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

void reload_modules(void) {
	xemil_t* cfg = NULL;

	prepare_config(&cfg);

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
