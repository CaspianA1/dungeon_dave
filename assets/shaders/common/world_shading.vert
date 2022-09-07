#version 400 core

out vec3 fragment_pos_world_space, UV, ambient_occlusion_UV;

uniform sampler3D ambient_occlusion_sampler;

vec3 get_ambient_occlusion_UV(const vec3 vertex_pos_world_space) {
	return (vertex_pos_world_space.xzy + 0.5f) / textureSize(ambient_occlusion_sampler, 0);
}
