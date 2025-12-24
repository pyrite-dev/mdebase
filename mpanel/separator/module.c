#include <Mw/Milsko.h>
#include <xemil.h>

void module(MwWidget box, xl_node_t* node) {
	MwVaCreateWidget(MwSeparatorClass, "separator", box, 0, 0, 0, 0,
			 MwNfixedSize, 10,
			 MwNorientation, MwVERTICAL,
			 NULL);
}
