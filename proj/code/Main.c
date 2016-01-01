#include <minix/sysutil.h>
#include <minix/syslib.h>
#include <minix/drivers.h>

#include "Utilities.h"
#include "WallyNIX.h"
#include "Graphics.h"
#include "Timer.h"
#include "Mouse.h"

FILE* logfd = NULL;
void initLog() {
	logfd = fopen(LOG_PATH, "w");
	LOG("initLog", "logging successfully initialized");
}

int main(int argc, char *argv[]) {
	initLog();
	LOG("main", "Starting Where's Wally? ...");

	/* Initialize service (synchronize with the RS) */
	sef_startup();
	printf("Starting WallyNIX.");

	initGraphics(MODE_1280_1024);

	WallyNIX* wally = (WallyNIX*) startWallyNIX();
	while (!wally->exit) {
		updateWallyNIX(wally);

		if (!wally->exit) {
			if (wally->draw) {
				drawWallyNIX(wally);
			}
			if(getMouse()->draw){
				copyToMouseBuffer();
				drawMouse();
			}
			copyToVideoMem();
		}
	}
	stopWallyNIX(wally);

	exitGraphics();

	printf("WallyNIX exited successfully\n");
	return EXIT_SUCCESS;
}