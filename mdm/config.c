#include "mdm.h"

#include <ini.h>

char* config_picture = NULL;

static int dumper(void* user, const char* section, const char* name, const char* value) {
	(void)user;

	if(strcmp(section, "MDM Config") != 0) return 1;

	if(strcmp(name, "Picture") == 0) config_picture = MDEStringDuplicate(value);

	return 1;
}

void parse_config(void) {
	ini_parse(CONFDIR "/mdm/mdmrc", dumper, NULL);
}
