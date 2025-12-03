#include "milkwm.h"

int main() {
	int st;

	if((st = init_x()) != 0) return st;

	loop_wm();
}
