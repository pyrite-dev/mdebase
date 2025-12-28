#include <MDE/Foundation.h>
#include <Mw/Milsko.h>
#include <xemil.h>

#include <stb_ds.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int calc_gap(MwWidget box) {
	return MwGetInteger(box, MwNheight) - 18 * 2;
}

static int calc_size(MwWidget box, xl_node_t* node) {
	xl_node_t* n;
	int	   gap = calc_gap(box);
	int	   c   = 0;

	n = node->first_child;
	while(n != NULL) {
		if(n->type == XL_NODE_NODE && strcmp(n->name, "Execute") == 0 && n->text != NULL) {
			c++;
		}

		n = n->next;
	}

	return (gap + 18) * ((c + (c % 2)) / 2) - gap;
}

static void exec_button(MwWidget handle, void* user, void* client){
	char** args = MDEStringToExec(MwGetText(handle, "Sexec"), NULL);
	int i;

	if(fork() == 0){
		execvp(args[0], args);
		_exit(-1);
	}

	for(i = 0; i < arrlen(args); i++) {
		if(args[i] != NULL) free(args[i]);
	}
	arrfree(args);
}

void module_reload(MwWidget box, MwWidget user, xl_node_t* node) {
	int	   w   = calc_size(box, node);
	int	   gap = calc_gap(box);
	xl_node_t* n;
	int	   c;
	MwWidget*  children;

	children = MwGetChildren(user);
	if(children != NULL) {
		int i;
		for(i = 0; children[i] != NULL; i++) {
			MwLLPixmap px = MwGetVoid(children[i], MwNpixmap);

			if(px != NULL) MwLLDestroyPixmap(px);

			MwDestroyWidget(children[i]);
		}
		free(children);
	}

	MwVaApply(user,
		  MwNfixedSize, w,
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

			btn = MwVaCreateWidget(MwButtonClass, "button", user, x, y, 18, 18,
					       MwNflat, 1,
					       MwNpixmap, px,
					       NULL);

			MwVaApply(btn,
				"Sexec", n->text,
			NULL);

			MwAddUserHandler(btn, MwNactivateHandler, exec_button, NULL);

			c++;
		}

		n = n->next;
	}
}

MwWidget module(MwWidget box, xl_node_t* node) {
	MwWidget f;
	int	 w = calc_size(box, node);

	f = MwVaCreateWidget(MwFrameClass, "launcher", box, 0, 0, 0, 0,
			     MwNfixedSize, w,
			     NULL);

	module_reload(box, f, node);

	return f;
}

void module_destroy(MwWidget box, MwWidget user) {
	MwWidget* children;

	children = MwGetChildren(user);
	if(children != NULL) {
		int i;
		for(i = 0; children[i] != NULL; i++) {
			MwLLPixmap px = MwGetVoid(children[i], MwNpixmap);

			if(px != NULL) MwLLDestroyPixmap(px);

			MwDestroyWidget(children[i]);
		}
		free(children);
	}

	MwDestroyWidget(user);
}
