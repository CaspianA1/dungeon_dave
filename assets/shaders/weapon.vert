#version 400 core

#include "common/shadow.vert"
#include "common/utils.vert"

noperspective out vec3 fragment_UV;

uniform uint frame_index;
uniform vec2 screen_corners[4];
uniform vec3 world_corners[4];

void main(void) {
	/* The z component of `gl_Position` here isn't
	`constants.weapon_sprite.ndc_dist_from_camera`
	in order to avoid any clipping */
	gl_Position = vec4(screen_corners[gl_VertexID], 0.0f, 1.0f);

	fragment_UV = vec3(gl_VertexID & 1, gl_VertexID < 2, frame_index);
	fragment_pos_light_space = world_space_transformation(world_corners[gl_VertexID], biased_light_model_view_projection).xyz;
}
