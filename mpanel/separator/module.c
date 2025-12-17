#include <Mw/Milsko.h>
#include <libconfig.h>

void module(MwWidget box, config_setting_t* setting) {
	MwVaCreateWidget(MwSeparatorClass, "separator", box, 0, 0, 0, 0,
			 MwNfixedSize, 10,
			 MwNorientation, MwVERTICAL,
			 NULL);
}
