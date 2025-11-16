#include "mdm.h"

#include <Mw/Milsko.h>

static void add_user(const char* name, void* user){
	MwComboBoxAdd(user, -1, name);
}

void login_window(void){
	MwWidget window, pic, userlabel, usercombo, passlabel, passentry, mainsep, sesslabel, sesscombo, fieldsep, shutdown, reboot, ok;
	MwLLPixmap p;

	MwLibraryInit();
	
	window = MwVaCreateWidget(MwWindowClass, "login", NULL, (x_width() - 366) / 2, (x_height() - 183) / 2, 366, 183,
		MwNhasBorder, 1,
		MwNinverted, 0,
	NULL);
	p = config_picture == NULL ? NULL : MwLoadImage(window, config_picture);
	pic = MwVaCreateWidget(MwImageClass, "image", window, 10, 10, 80, 137,
		MwNhasBorder, 1,
	NULL);
	userlabel = MwVaCreateWidget(MwLabelClass, "userlabel", window, 95, 10, MwTextWidth(window, "User:"), 16,
		MwNtext, "User:",
	NULL);
	usercombo = MwCreateWidget(MwComboBoxClass, "usercombo", window, 95, 10+16+5, 265, 18);
	passlabel = MwVaCreateWidget(MwLabelClass, "passlabel", window, 95, 10+16+5+18+5, MwTextWidth(window, "Password:"), 16,
		MwNtext, "Password:",
	NULL);
	passentry = MwVaCreateWidget(MwEntryClass, "passentry", window, 95, 10+16+5+18+5+16+5, 265, 18,
		MwNhideInput, 1,
	NULL);
	mainsep = MwVaCreateWidget(MwSeparatorClass, "mainsep", window, 95, 10+16+5+18+5+16+5+18, 265, 10,
		MwNorientation, MwHORIZONTAL,
	NULL);
	sesslabel = MwVaCreateWidget(MwLabelClass, "sesslabel", window, 95, 10+16+5+18+5+16+5+18+10, MwTextWidth(window, "Session Type:"), 16,
		MwNtext, "Session Type:",
	NULL);
	sesscombo = MwCreateWidget(MwComboBoxClass, "sesscombo", window, 95, 10+16+5+18+5+16+5+18+10+16+5, 265, 18);
	fieldsep = MwVaCreateWidget(MwSeparatorClass, "fieldsep", window, (366 - 362) / 2, 10+137, 362, 10,
		MwNorientation, MwHORIZONTAL,
	NULL);
	shutdown = MwVaCreateWidget(MwButtonClass, "shutdown", window, 10, 10+137+10, 80, 18,
		MwNtext, "Shutdown",
	NULL);
	reboot = MwVaCreateWidget(MwButtonClass, "reboot", window, 10+80+5, 10+137+10, 70, 18,
		MwNtext, "Reboot",
	NULL);
	ok = MwVaCreateWidget(MwButtonClass, "ok", window, 366 - 10 - 60, 10+137+10, 60, 18,
		MwNtext, "OK",
	NULL);

	if(p != NULL) MwVaApply(pic, MwNpixmap, p, NULL);

	MDEListUsers(add_user, usercombo);

	MwLoop(window);
}
