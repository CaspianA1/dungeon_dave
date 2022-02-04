/*
- This demo focuses on shadow volumes for sectors

Decision to make (one to choose):
- One shadow volume per sector?
- One per face? If so, per overall faces or per faces split by other sectors?
*/

#include "../utils.c"
#include "../sector.c"

void sector_shadow_visibility(const Sector* const sector, const vec3 light_pos) {
	const byte *const origin = sector -> origin, *const size = sector -> size;

	// These are top-down values
	const byte start_x = origin[0], start_y = origin[1];
	const byte end_x = start_x + size[0], end_y = start_y + size[1];

	if (light_pos[1] > sector -> visible_heights.max) {
		puts("Flat face visible");
	}

	if (light_pos[0] < start_x) {
		puts("Left vert NS face visible");
	}
	else if (light_pos[0] > end_x) {
		puts("Right vert NS face visible");
	}

	if (light_pos[2] < start_y) {
		puts("Top vert EW face visible");
	}
	else if (light_pos[2] > end_y) {
		puts("Bottom vert EW face visible");
	}

	// The next step is to take all invisible faces, and find the ones that are adjacent to visible faces
}

#ifdef DEMO_23
int main(void) {

}
#endif
