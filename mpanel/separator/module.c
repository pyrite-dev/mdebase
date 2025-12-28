#include <Mw/Milsko.h>
#include <xemil.h>

MwWidget module(MwWidget box, xl_node_t* node) {
	return MwVaCreateWidget(MwSeparatorClass, "separator", box, 0, 0, 0, 0,
				MwNfixedSize, 10,
				MwNorientation, MwVERTICAL,
				NULL);
}

void module_destroy(MwWidget box, MwWidget user) {
	MwDestroyWidget(user);
}
