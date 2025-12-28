#include <Mw/Milsko.h>
#include <xemil.h>

MwWidget module(MwWidget box, xl_node_t* node) {
	MwWidget b = MwCreateWidget(MwFrameClass, "taskbar", box, 0, 0, 0, 0);

	return b;
}

void module_destroy(MwWidget box, MwWidget user) {
	MwDestroyWidget(user);
}
