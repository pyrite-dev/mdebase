#include "milkwm.h"

void rehash(int sig) {
	pthread_mutex_lock(&xmutex);
	wm_rehash = 1;
	pthread_mutex_unlock(&xmutex);
}

int main() {
	int st;

	wm_config_init();
	wm_config_read();

	if((st = init_x()) != 0) return st;

	signal(SIGHUP, rehash);

	loop_wm();
}
