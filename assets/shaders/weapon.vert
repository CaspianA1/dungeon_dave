#version 400 core

#include "csm/shadow.vert"

out vec3 fragment_pos_world_space;
noperspective out vec3 fragment_UV;

uniform uint frame_index;
uniform vec3 world_corners[4];
uniform mat4 view_projection;

void main(void) {
	/* The z component of `gl_Position` here isn't
	`constants.weapon_sprite.ndc_dist_from_camera`
	in order to avoid any clipping */
	fragment_UV = vec3(gl_VertexID & 1, gl_VertexID < 2, frame_index);

	fragment_pos_world_space = world_corners[gl_VertexID];

	gl_Position = view_projection * vec4(fragment_pos_world_space, 1.0f);
}
