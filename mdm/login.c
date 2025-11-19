#include "mdm.h"

#include <ini.h>
#include <sched.h>
#include <security/_pam_types.h>
#include <security/pam_appl.h>
#include <stb_ds.h>
#include <stdlib.h>
#include <string.h>

MwWidget root;
MwWidget window, sesscombo, usercombo, passentry, error;

typedef struct session {
	char*		       key;
	session_environment_t* value;
} session_t;

session_t* sessions	      = NULL;
char*	   name_line	      = NULL;
char*	   try_exec_line      = NULL;
char*	   exec_line	      = NULL;
char*	   desktop_names_line = NULL;

static void add_user(const char* name, void* user) {
	MwComboBoxAdd(user, -1, name);
}

static int dumper(void* user, const char* section, const char* name, const char* value) {
	if(strcmp(section, "Desktop Entry") != 0) return 1;

	if(strcmp(name, "Name") == 0) {
		name_line = MDEStringDuplicate(value);
	} else if(strcmp(name, "TryExec") == 0) {
		try_exec_line = MDEStringDuplicate(value);
	} else if(strcmp(name, "Exec") == 0) {
		exec_line = MDEStringDuplicate(value);
	} else if(strcmp(name, "DesktopNames") == 0) {
		desktop_names_line = MDEStringDuplicate(value);
	}

	return 1;
}

static void add_session(session_type_t session_type, const char* path, int dir, int symlink, void* user) {
	int		       m     = 1;
	session_environment_t* value = malloc(sizeof(session_environment_t));

	name_line	   = NULL;
	exec_line	   = NULL;
	try_exec_line	   = NULL;
	desktop_names_line = NULL;

	if(dir || symlink) return;

	ini_parse(path, dumper, user);

	if(name_line != NULL && (exec_line != NULL || try_exec_line != NULL)) {
		int i;
		for(i = 0; i < shlen(sessions); i++) {
			if(strcmp(sessions[i].key, name_line) == 0) break;
		}

		if(i == shlen(sessions)) {
			MwComboBoxAdd(user, -1, name_line);

			value->run	     = exec_line;
			value->try_run	     = try_exec_line;
			value->session_type  = session_type;
			value->desktop_names = desktop_names_line;
			shput(sessions, name_line, value);

			m = 0;
		}
	}

	if(name_line != NULL) free(name_line);
	if(desktop_names_line != NULL) free(desktop_names_line);
	if(m && try_exec_line != NULL) free(try_exec_line);
	if(m && exec_line != NULL) free(exec_line);
}

static void add_session_x(const char* path, int dir, int symlink, void* user) {
	add_session(MDM_SESSION_TYPE_X, path, dir, symlink, user);
}
static void add_session_wayland(const char* path, int dir, int symlink, void* user) {
	add_session(MDM_SESSION_TYPE_WAYLAND, path, dir, symlink, user);
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
	const char*	username;
	const char*	place;
	struct pam_conv conv;
	pam_handle_t*	pam;
	int		err;
	int		err2;
	int		ret;
	char		err_str[255];
	int		print_error = 0;
	int		i, n;

	username	 = MwComboBoxGet(usercombo, MwGetInteger(usercombo, MwNvalue));
	conv.conv	 = conversation;
	conv.appdata_ptr = NULL;
	err		 = 1;

	(void)handle;
	(void)user;
	(void)client;

	ret = pam_start("mdm", username, &conv, &pam);
	if(ret != PAM_SUCCESS) {
		place = "Error calling pam_start";
		goto cleanup;
	}

	ret = pam_authenticate(pam, 0);
	if(ret != PAM_SUCCESS) {
		place = "Error calling pam_authenticate";
		goto cleanup;
	}

	ret = pam_acct_mgmt(pam, 0);
	if(ret != PAM_SUCCESS) {
		place = "Error calling pam_acct_mgmt";
		goto cleanup;
	}

	ret = pam_open_session(pam, 0);
	if(ret != PAM_SUCCESS) {
		place = "Error calling pam_open_session";
		goto cleanup;
	}
cleanup:
	pam_close_session(pam, 0);

	// if pam_end fails but we have another error, we care more about the error at hand
	// then whether pam successfully ended.
	err2 = pam_end(pam, 0);

	if(err && ret != PAM_SUCCESS) {
		MwSetText(passentry, MwNtext, "");
		snprintf(err_str, 255, "%s: %s", place, pam_strerror(pam, err));
		print_error = 1;
	} else if(err2) {
		MwSetText(passentry, MwNtext, "");
		snprintf(err_str, 255, "Error calling pam_end: %s", pam_strerror(pam, err2));
		print_error = 1;
	}
	if(print_error) {
		n = 0;
		for(i = 0; err_str[i] != '\0' && i <= 255; i++ && n++) {
			if(err_str[i] == ' ' && n >= 40) {
				err_str[i] = '\n';
				n	   = 0;
			}
		}
		MwSetText(error, MwNtext, err_str);
	} else {
		printf("Logging in as %s\n", username);
		struct passwd* pwd     = getpwnam(username);
		const char*    session = MwComboBoxGet(sesscombo, MwGetInteger(sesscombo, MwNvalue));

		uid = pwd->pw_uid;
		gid = pwd->pw_gid;
		env = shget(sessions, session);

		MwDestroyWidget(root);
	}
}

void login_window(void) {
	MwWidget   pic, userlabel, passlabel, mainsep, sesslabel, fieldsep, shutdown, reboot, ok;
	MwLLPixmap p;
	void*	   out;
	int	   i;

	pthread_mutex_lock(&xmutex);

	MwLibraryInit();

	root = MwCreateWidget(NULL, "root", NULL, 0, 0, 0, 0);

	window	  = MwVaCreateWidget(MwWindowClass, "login", root, (x_width() - 366) / 2, (x_height() - 200) / 2, 366, 250,
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
	error	  = MwVaCreateWidget(MwLabelClass, "error", window, 10, 10 + 137 + 10 + 20, 366, 48, NULL);
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
	MDEDirectoryScan(DATADIR "/xsessions", add_session_x, sesscombo);
	MDEDirectoryScan("/usr/share/xsessions", add_session_x, sesscombo);
	MDEDirectoryScan(DATADIR "/wayland-sessions", add_session_wayland, sesscombo);
	MDEDirectoryScan("/usr/share/wayland-sessions", add_session_wayland, sesscombo);

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
