#include <MDE/Foundation.h>
#include <Mw/Milsko.h>
#include <xemil.h>

void module(MwWidget box, xl_node_t* node) {
	MwWidget   f;
	xl_node_t* n;
	int	   c;
	int	   w;
	int	   gap = (MwGetInteger(box, MwNheight) - 18 * 2);

	c = 0;
	n = node->first_child;
	while(n != NULL) {
		if(n->type == XL_NODE_NODE && strcmp(n->name, "Execute") == 0 && n->text != NULL) {
			c++;
		}

		n = n->next;
	}

	w = (c + (c % 2)) / 2;

	f = MwVaCreateWidget(MwFrameClass, "launcher", box, 0, 0, 0, 0,
			     MwNfixedSize, (gap + 18) * w - gap,
			     NULL);

	c = 0;
	n = node->first_child;
	while(n != NULL) {
		int x = (gap + 18) * (c / 2);
		int y = (gap + 18) * (c % 2);

		if(n->type == XL_NODE_NODE && strcmp(n->name, "Execute") == 0 && n->text != NULL) {
			MwLLPixmap	px = NULL;
			MwWidget	btn;
			const char*	icon = NULL;
			xl_attribute_t* a    = n->first_attribute;

			while(a != NULL) {
				if(strcmp(a->key, "Icon") == 0) {
					icon = a->value;
				}
				a = a->next;
			}

			if(icon != NULL && icon[0] == '/') {
				px = MwLoadImage(box, icon);
			} else if(icon != NULL) {
				char* p = MDEIconLookUp("Applications", icon, 16);

				if(p != NULL) {
					px = MwLoadImage(box, p);
					free(p);
				}
			}

			btn = MwVaCreateWidget(MwButtonClass, "button", f, x, y, 18, 18,
					       MwNflat, 1,
					       MwNpixmap, px,
					       NULL);

			c++;
		}

		n = n->next;
	}
}
