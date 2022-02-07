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

	DEBUG_BITS(light_visiblity_bits);

	/*
	- The next step is to take all invisible faces, and find the ones that are adjacent to visible faces.
	- In other words, find shadowed faces that are next to unshadowed faces, and then collect those shared vertices.
	- And in other words too, all instances where a 0 bit is followed by a 1 bit, or the opposite.
	- And for flat face, if visible, all top edge vertices visible (but not necessarily all in shadow volume)

	For a flat face:
		- If flat face in shadow, 4 vertices from top are part of shadow volume

	For a vert face:
		- If vert face in shadow, 4 vertices from side are part of shadow volume

	- Since flat faces are more complicated, come back to them later

	-----

	- 8 possible vertices for a sector/cuboid
	- Store those vertices in a byte as a vertex set (8 possible states)

	Cuboid corner ids (from a top-down perspective):
		0 = shallow top left, 1 = shallow top right,
		2 = shallow bottom left, 3 = shallow bottom right,

		4 = deep top left, 5 = deep top right,
		6 = deep bottom left, 7 = deep bottom right

	A mapping from faces to corner id sets:
		flat = {0, 1, 2, 3},
		left vert ns = {0, 2, 4, 6},
		top vert ew = {0, 1, 4, 5},
		right vert ns = {1, 3, 5, 7},
		bottom vert ew = {2, 3, 6, 7}
	-----

	With finding visibility, how do flat faces play a part?
	*/

	if (bit_is_set(light_visiblity_bits, 1)) {
		puts("Flat is visible");
		// All 4 edges may not be part of shadow volume
	}

	for (byte face_id = 1; face_id < 5; face_id++) {
		// `& 3` makes the next face id wrap around to 0 if the face id is 4. Used b/c adjacent value of id 4 is 1.
		const byte next_face_id = (face_id & 3) + 1;

		const byte
			curr_vert_face_in_light = !!bit_is_set(light_visiblity_bits, 1 << face_id),
			next_vert_face_in_light = !!bit_is_set(light_visiblity_bits, 1 << next_face_id);

		// If one face is in light and not the other, that indicates a silhouette transition
		if (curr_vert_face_in_light ^ next_vert_face_in_light) {
			printf("Silhouette from face id %d to %d\n", face_id, next_face_id);
		}
	}
}

#ifdef DEMO_23
int main(void) {
	const Sector sector = {
		.texture_id = 255, // Not used
		.origin = {4, 5}, .size = {2, 3},
		.visible_heights = {.min = 3, .max = 8},
		.face_range = {.start = 0, .length = 0} // Not used
	};

	const vec3 light_pos = {3.5f, 8.0001f, 4.5f};

	sector_shadow_visibility(&sector, light_pos);
}
#endif
