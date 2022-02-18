#ifndef SHADERS_C
#define SHADERS_C

#include "../headers/utils.h"

const GLchar *const sector_vertex_shader =
	"#version 330 core\n"

	"layout(location = 0) in vec3 vertex_pos_world_space;\n"
	"layout(location = 1) in int face_info_bits;\n"

	"out vec2 lightmap_UV;\n"
	"out vec3 UV, face_normal, pos_delta_world_space;\n"

	"uniform ivec2 map_size;\n"
	"uniform vec3 camera_pos_world_space;\n"
	"uniform mat4 model_view_projection;\n"

	"const vec3 face_normals[5] = vec3[5](\n"
		"vec3(0.0f, 1.0f, 0.0f), vec3(1.0f, 0.0f, 0.0f),\n" // Flat, right, bottom, left, top
		"vec3(0.0f, 0.0f, 1.0f), vec3(-1.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, -1.0f)\n"
	");\n"

	"const ivec2 uv_indices[5] = ivec2[5](\n"
		"ivec2(0, 2), ivec2(2, 1), ivec2(0, 1), ivec2(2, 1), ivec2(0, 1)\n"
	"),\n"

	"uv_signs[5] = ivec2[5](\n"
		"ivec2(1, 1), ivec2(-1, -1), ivec2(1, -1), ivec2(1, -1), ivec2(-1, -1)\n"
	");\n"

	"void set_normal_and_UV_from_face_id(int face_id_bits) {\n"
		"face_normal = face_normals[face_id_bits];\n"

		"ivec2 uv_index = uv_indices[face_id_bits];\n"
		"vec2 UV_xy = uv_signs[face_id_bits] * vec2(vertex_pos_world_space[uv_index.x], vertex_pos_world_space[uv_index.y]);\n"

		"UV = vec3(UV_xy, face_info_bits >> 3);\n"

	"}\n"

	"void main(void) {\n"
		"gl_Position = model_view_projection * vec4(vertex_pos_world_space, 1.0f);\n"
		"set_normal_and_UV_from_face_id(face_info_bits & 7);\n"
		"lightmap_UV = vertex_pos_world_space.xz / map_size;\n"
		"pos_delta_world_space = camera_pos_world_space - vertex_pos_world_space;\n"
	"}\n",

*const sector_fragment_shader =
    "#version 330 core\n"

	"in vec2 lightmap_UV;\n"
	"in vec3 UV, face_normal, pos_delta_world_space;\n"

	"out vec3 color;\n"

	"uniform float ambient_strength, diffuse_strength, attenuation_factor;\n"
	"uniform sampler2D lightmap_sampler;\n"
	"uniform sampler2DArray texture_sampler;\n"

	"float diffuse(void) {\n" // Faces get darker as the view angle from it gets steeper
		"vec3 light_dir = normalize(pos_delta_world_space);\n"
		"return dot(light_dir, face_normal) * diffuse_strength;\n"
	"}\n"

	"float attenuation(void) {\n" // Distance-based lighting
		"float dist_squared = dot(pos_delta_world_space, pos_delta_world_space);\n"
		"return 1.0f / (1.0f + attenuation_factor * dist_squared);\n"
	"}\n"

	"float calculate_light(void) {\n"
		"float lightmap_ambient = texture(lightmap_sampler, lightmap_UV).r;\n"
		"float light = ((lightmap_ambient + ambient_strength + diffuse())) * attenuation();\n"
		"return min(light, 1.0f);\n"
	"}\n"

	"void main(void) {\n"
		"color = texture(texture_sampler, UV).rgb * calculate_light();\n"
	"}\n",

*const billboard_vertex_shader =
	"#version 330 core\n"

	"layout(location = 0) in uint in_texture_id;\n"
	"layout(location = 1) in vec2 billboard_size_world_space;\n"
	"layout(location = 2) in vec3 billboard_center_world_space;\n"

	"out float texture_id;\n"
	"out vec2 UV;\n"

	"uniform vec2 right_xz_world_space;\n"
	"uniform mat4 model_view_projection;\n"

	"const vec2 vertices_model_space[4] = vec2[4](\n"
		"vec2(-0.5f, -0.5f), vec2(0.5f, -0.5f),\n"
		"vec2(-0.5f, 0.5f), vec2(0.5f, 0.5f)\n"
	");\n"

	"void main(void) {\n"
		"vec2 vertex_model_space = vertices_model_space[gl_VertexID];\n"
		"vec2 upscaled_vertex_world_space = vertex_model_space * billboard_size_world_space;\n"

		"vec3 vertex_world_space = billboard_center_world_space\n"
			"+ upscaled_vertex_world_space.x * vec3(right_xz_world_space, 0.0f).xzy\n"
			"+ vec3(0.0f, upscaled_vertex_world_space.y, 0.0f);\n"

		"gl_Position = model_view_projection * vec4(vertex_world_space, 1.0f);\n"

		"texture_id = in_texture_id;\n"
		"UV = vec2(vertex_model_space.x, -vertex_model_space.y) + 0.5f;\n"
	"}\n",

*const billboard_fragment_shader =
    "#version 330 core\n"

	"in float texture_id;\n"
	"in vec2 UV;\n"

	"out vec4 color;\n"

	"uniform sampler2DArray texture_sampler;\n"

	"void main(void) {\n"
		"color = texture(texture_sampler, vec3(UV, texture_id));\n"
		"if (color.a < 0.28f) discard;\n" // 0.28f empirically tested for best discarding of alpha value
	"}\n",

*const skybox_vertex_shader =
	"#version 330 core\n"

	"layout(location = 0) in vec3 vertex_pos_world_space;\n"

	"out vec3 UV_3D;\n"

	"uniform mat4 model_view_projection;\n"

	"void main(void) {\n"
		"gl_Position = (model_view_projection * vec4(vertex_pos_world_space, 1.0f)).xyww;\n"
		"UV_3D = vertex_pos_world_space;\n"
		"UV_3D.x = -UV_3D.x;\n" // Without this, the X component of the UV is reversed
	"}\n",

*const skybox_fragment_shader =
    "#version 330 core\n"

	"in vec3 UV_3D;\n"

	"out vec3 color;\n"

	"uniform samplerCube texture_sampler;\n"

	"void main(void) {\n"
		"color = texture(texture_sampler, UV_3D).rgb;\n"
	"}\n",

*const water_vertex_shader =
	"#version 330 core\n"
	"#define sin_cycles 6.28f\n"
	"#define speed 2.0f\n"

	"out vec2 UV, angles;\n"

	"uniform float time;\n" // In seconds

	// Bottom left, bottom right, top left, top right
	"const vec2 corners[4] = vec2[4] (\n"
		"vec2(-1, -1), vec2(1, -1), vec2(-1, 1), vec2(1, 1)\n"
	");\n"

	"void main(void) {\n"
		"vec2 vertex_pos = corners[gl_VertexID];\n"
		"gl_Position = vec4(vertex_pos, 0.0f, 1.0f);\n"
		"UV = vertex_pos * vec2(0.5f, -0.5f) + 0.5f;\n"
		"angles = UV.yx * sin_cycles + time * speed;\n"
	"}\n",

*const water_fragment_shader =
	"#version 330 core\n"
	"#define dist_mag 0.05f\n"

	"in vec2 UV, angles;\n"

	"out vec3 color;\n"

	"uniform sampler2D texture_sampler;\n"

	"void main(void) {\n"
		"vec2 wavy_UV = UV + sin(angles) * dist_mag;\n"
		"color = texture(texture_sampler, wavy_UV).rgb;\n"
	"}\n";

#endif
