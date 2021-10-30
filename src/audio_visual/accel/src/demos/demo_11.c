#include "demo_10.c"

enum {map_width = 8, map_height = 5};

const byte heightmap[map_height][map_width] = {
	{0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 1, 2, 0, 0, 0},
	{0, 0, 0, 1, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0}
};

#ifdef DEMO_11
int main(void) {
    make_application(demo_7_drawer, demo_7_init, deinit_demo_vars);
}
#endif
