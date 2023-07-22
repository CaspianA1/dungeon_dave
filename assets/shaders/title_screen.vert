#version 400 core

#include "common/quad_utils.vert"

noperspective out float scrolling_UV_x;
noperspective out vec2 UV;
noperspective out vec3 fragment_pos_tangent_space;

uniform float scroll_factor, scrolling_texture_vert_squish_ratio;
uniform vec3 light_pos_tangent_space;

uniform sampler2DArray scrolling_albedo_sampler;

void main(void) {
	UV = get_quad_UV();
	scrolling_UV_x = UV.x;

	ivec2 scrolling_texture_size = textureSize(scrolling_albedo_sampler, 0).xy;
	float scrolling_aspect_ratio = float(scrolling_texture_size.x) / scrolling_texture_size.y;

	// Making the texture aspect ratio correct
	scrolling_UV_x /= scrolling_aspect_ratio * scrolling_texture_vert_squish_ratio;
	scrolling_UV_x += scroll_factor;

	gl_Position = vec4(quad_corners[gl_VertexID], 0.0f, 1.0f);
	fragment_pos_tangent_space = vec3(-gl_Position.x, gl_Position.y, 0.0f);
}
