#include "mdm.h"

#include <ini.h>
#include <security/pam_appl.h>
#include <stb_ds.h>

MwWidget root;
MwWidget window, sesscombo, usercombo, passentry;

typedef struct session {
	char* key;
	char* value;
} session_t;

session_t* sessions = NULL;
char*	   name_line;
char*	   exec_line;

static void add_user(const char* name, void* user) {
	MwComboBoxAdd(user, -1, name);
}

static int dumper(void* user, const char* section, const char* name, const char* value) {
	if(strcmp(section, "Desktop Entry") != 0) return 1;

	if(strcmp(name, "Name") == 0) {
		MwComboBoxAdd(user, -1, value);

		name_line = MDEStringDuplicate(value);
	} else if(strcmp(name, "Exec") == 0) {
		exec_line = MDEStringDuplicate(value);
	}

	return 1;
}

static void add_session(const char* path, int dir, void* user) {
	name_line = NULL;
	exec_line = NULL;

	ini_parse(path, dumper, user);

	if(name_line != NULL && exec_line != NULL) {
		shput(sessions, name_line, exec_line);
	}

	if(name_line != NULL) free(name_line);
}

static int conversation(int num_msg, const struct pam_message** msg, struct pam_response** resp, void* appdata) {
	int i;

	*resp = malloc(num_msg * sizeof(struct pam_response));

	for(i = 0; i < num_msg; i++) {
		if(msg[i]->msg_style == PAM_PROMPT_ECHO_OFF) {
			const char* txt = MwGetText(passentry, MwNtext);
			if(txt == NULL) txt = "";

			resp[i]->resp_retcode = PAM_SUCCESS;
			resp[i]->resp	      = MDEStringDuplicate(txt);
		} else {
			return PAM_CONV_ERR;
		}
	}

	return PAM_SUCCESS;
}

static void try_login(MwWidget handle, void* user, void* client) {
	const char*	username = MwComboBoxGet(usercombo, MwGetInteger(usercombo, MwNvalue));
	struct pam_conv conv	 = {conversation, NULL};
	pam_handle_t*	pam;
	int		ret;
	jmp_buf		cleanup;
	int		err = 1;

	(void)handle;
	(void)user;
	(void)client;

	if(setjmp(cleanup)) {
		pam_end(pam, 0);

		if(err) {
			MwSetText(passentry, MwNtext, "");
		} else {
			struct passwd* pwd     = getpwnam(username);
			const char*    session = MwComboBoxGet(sesscombo, MwGetInteger(sesscombo, MwNvalue));

			uid = pwd->pw_uid;
			gid = pwd->pw_gid;
			run = shget(sessions, session);

			MwDestroyWidget(root);
		}

		return;
	}

	ret = pam_start("mdm", username, &conv, &pam);
	if(ret != PAM_SUCCESS) {
		longjmp(cleanup, 1);
	}

	ret = pam_authenticate(pam, 0);
	if(ret != PAM_SUCCESS) {
		longjmp(cleanup, 1);
	}

	ret = pam_acct_mgmt(pam, 0);
	if(ret != PAM_SUCCESS) {
		longjmp(cleanup, 1);
	}

	err = 0;
	longjmp(cleanup, 1);
}

void login_window(void) {
	MwWidget   pic, userlabel, passlabel, mainsep, sesslabel, fieldsep, shutdown, reboot, ok;
	MwLLPixmap p;
	void*	   out;
	int	   i;

	pthread_mutex_lock(&xmutex);

	MwLibraryInit();

	root = MwCreateWidget(NULL, "root", NULL, 0, 0, 0, 0);

	window	  = MwVaCreateWidget(MwWindowClass, "login", root, (x_width() - 366) / 2, (x_height() - 183) / 2, 366, 183,
				     MwNtitle, "login",
				     NULL);
	p	  = config_picture == NULL ? NULL : MwLoadImage(window, config_picture);
	pic	  = MwVaCreateWidget(MwImageClass, "image", window, 10, 10, 80, 137,
				     MwNhasBorder, 1,
				     NULL);
	userlabel = MwVaCreateWidget(MwLabelClass, "userlabel", window, 95, 10, MwTextWidth(window, "User:"), 16,
				     MwNtext, "User:",
				     NULL);
	usercombo = MwCreateWidget(MwComboBoxClass, "usercombo", window, 95, 10 + 16 + 5, 265, 18);
	passlabel = MwVaCreateWidget(MwLabelClass, "passlabel", window, 95, 10 + 16 + 5 + 18 + 5, MwTextWidth(window, "Password:"), 16,
				     MwNtext, "Password:",
				     NULL);
	passentry = MwVaCreateWidget(MwEntryClass, "passentry", window, 95, 10 + 16 + 5 + 18 + 5 + 16 + 5, 265, 18,
				     MwNhideInput, 1,
				     NULL);
	mainsep	  = MwVaCreateWidget(MwSeparatorClass, "mainsep", window, 95, 10 + 16 + 5 + 18 + 5 + 16 + 5 + 18, 265, 10,
				     MwNorientation, MwHORIZONTAL,
				     NULL);
	sesslabel = MwVaCreateWidget(MwLabelClass, "sesslabel", window, 95, 10 + 16 + 5 + 18 + 5 + 16 + 5 + 18 + 10, MwTextWidth(window, "Session Type:"), 16,
				     MwNtext, "Session Type:",
				     NULL);
	sesscombo = MwCreateWidget(MwComboBoxClass, "sesscombo", window, 95, 10 + 16 + 5 + 18 + 5 + 16 + 5 + 18 + 10 + 16 + 5, 265, 18);
	fieldsep  = MwVaCreateWidget(MwSeparatorClass, "fieldsep", window, 10, 10 + 137, 351, 10,
				     MwNorientation, MwHORIZONTAL,
				     NULL);
	shutdown  = MwVaCreateWidget(MwButtonClass, "shutdown", window, 10, 10 + 137 + 10, 80, 18,
				     MwNtext, "Shut Down",
				     NULL);
	reboot	  = MwVaCreateWidget(MwButtonClass, "reboot", window, 10 + 80 + 5, 10 + 137 + 10, 70, 18,
				     MwNtext, "Reboot",
				     NULL);
	ok	  = MwVaCreateWidget(MwButtonClass, "ok", window, 366 - 10 - 60, 10 + 137 + 10, 60, 18,
				     MwNtext, "OK",
				     NULL);

	if(p != NULL) MwVaApply(pic, MwNpixmap, p, NULL);

	MwAddUserHandler(ok, MwNactivateHandler, try_login, NULL);
	MwAddUserHandler(passentry, MwNactivateHandler, try_login, NULL);

	MDEUsersList(add_user, usercombo);

	for(i = 0; i < shlen(sessions); i++) {
		free(sessions[i].value);
	}
	shfree(sessions);
	sh_new_strdup(sessions);
	shdefault(sessions, NULL);
	MDEDirectoryScan(DATADIR "/xsessions", add_session, sesscombo);
	MDEDirectoryScan("/usr/share/xsessions", add_session, sesscombo);

	pthread_mutex_unlock(&xmutex);

	while(1) {
		int s = 0;

		pthread_mutex_lock(&xmutex);
		while(MwPending(root)) {
			if((s = MwStep(root)) != 0) break;
		}
		pthread_mutex_unlock(&xmutex);
		if(s != 0) break;

		MwTimeSleep(30);
	}

	pthread_join(xthread, &out);
}
