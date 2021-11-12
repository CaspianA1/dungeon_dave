#include "../utils.c"

void culling_loop(void) {

}

// #ifdef DEMO_17
int main(void) {
	Screen screen = init_screen("Culling Demo");

	const int max_delay = 1000 / constants.fps;
	(void) max_delay;

	deinit_screen(&screen);
}
// #endif
