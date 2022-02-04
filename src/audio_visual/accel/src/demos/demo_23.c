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
	const GLfloat light_pos_x_top_down = light_pos[0], light_pos_y_top_down = light_pos[2];

	/*
	Face ids:
	0 = flat, 1 = left vert ns, 2 = top vert ew, 3 = right vert ns, 4 = bottom vert ew.
	In the bits below, if a bit is set at index i, the face i is in the light.
	*/

	const byte light_visiblity_bits =
		(light_pos[1] > sector -> visible_heights.max) | // Flat
		((light_pos_x_top_down < start_x) << 1) | // Left vert NS
		((light_pos_y_top_down < start_y) << 2) | // Top vert EW
		((light_pos_x_top_down > end_x) << 3) | // Right vert NS
		((light_pos_y_top_down > end_y) << 4); // Bottom vert EW

	(void) light_visiblity_bits;

	/*
	- The next step is to take all invisible faces, and find the ones that are adjacent to visible faces.
	- In other words, find shadowed faces that are next to unshadowed faces, and then collect those shared vertices.
	- And in other words too, all instances where a 0 bit is followed by a 1 bit, or the opposite.
	*/
}

#ifdef DEMO_23
int main(void) {

}
#endif
