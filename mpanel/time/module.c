#include <Mw/Milsko.h>
#include <libconfig.h>

#include <time.h>
#include <string.h>

static time_t last;

static void tick(MwWidget handle, void* user, void* client) {
	MwWidget*  children = MwGetChildren(handle);
	int	   i;
	time_t	   t = time(NULL);
	struct tm* tm;

	if(t == last) return;
	last = t;

	tm = localtime(&t);

	for(i = 0; children[i] != NULL; i++) {
		const char* n = MwGetName(children[i]);
		if(strcmp(n, "seg") == 0) {
			char buf[16];

			sprintf(buf, "%02d:%02d", tm->tm_hour, tm->tm_min);

			MwVaApply(children[i],
				  MwNtext, buf,
				  NULL);
		} else if(strcmp(n, "date") == 0) {
			char	    buf[16];
			const char* mon[] = {
			    "Jan",
			    "Feb",
			    "Mar",
			    "Apr",
			    "May",
			    "Jun",
			    "Jul",
			    "Aug",
			    "Sep",
			    "Oct",
			    "Nov",
			    "Dec"};

			sprintf(buf, "%s %.2d", mon[tm->tm_mon], tm->tm_mday);

			MwVaApply(children[i],
				  MwNtext, buf,
				  NULL);
		}
	}

	free(children);
}

void module(MwWidget box, config_setting_t* setting) {
	MwWidget b    = MwVaCreateWidget(MwBoxClass, "time", box, 0, 0, 0, 0,
					 MwNfixedSize, 64,
					 MwNbackground, "#bebdb2",
					 MwNforeground, "#484841",
					 MwNorientation, MwVERTICAL,
					 MwNhasBorder, 1,
					 MwNinverted, 1,
					 MwNborderWidth, 1,
					 NULL);
	MwWidget seg  = MwVaCreateWidget(MwLabelClass, "seg", b, 0, 0, 0, 0,
					 MwNsevenSegment, 1,
					 MwNtext, "00:00",
					 MwNratio, 5,
					 NULL);
	MwWidget date = MwVaCreateWidget(MwLabelClass, "date", b, 0, 0, 0, 0,
					 MwNtext, "??? ??",
					 MwNratio, 3,
					 NULL);

	last = 0;
	tick(b, NULL, NULL);

	MwAddUserHandler(b, MwNtickHandler, tick, NULL);

	MwAddTickList(b);
}
