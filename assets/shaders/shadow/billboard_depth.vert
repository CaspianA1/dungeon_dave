#version 400 core

#include "../common/billboard_transform.vert"
#include "../common/shared_params.glsl"

flat out vec3 vertex_UV;

void main(void) {
	vertex_UV = vec3(get_quad_UV(), billboard_texture_id);
	gl_Position = vec4(get_billboard_vertex(-billboard_front_facing_tbn[0]), 1.0f);
}
