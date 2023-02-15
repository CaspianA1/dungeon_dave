#version 400 core

uniform samplerCube equiangular_skybox_sampler;

in vec3 rotated_fragment_pos_model_space;

out vec3 color;

void main(void) {
	/* TODO: perhaps don't read from a cubemap here, but another texture, to stop the cubiness.
	- For this, define a vertex type, assign UVs to each vertex, and put the cubemap faces in an array texture
	(note: the specific face subtexture will be indexed by `gl_Layer` (or `gl_InstanceID`, not sure) */
	color = texture(equiangular_skybox_sampler, rotated_fragment_pos_model_space).rgb;
}
