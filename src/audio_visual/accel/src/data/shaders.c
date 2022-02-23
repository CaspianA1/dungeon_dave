#ifndef SHADERS_C
#define SHADERS_C

#include "../headers/utils.h"

const GLchar *const sector_vertex_shader =
	"#version 330 core\n"

	"layout(location = 0) in vec3 vertex_pos_world_space;\n"
	"layout(location = 1) in int face_info_bits;\n"

	"out vec3 UV, face_normal, light_pos_delta_world_space;\n"
	"out vec4 fragment_pos_light_space;\n"

	"uniform float shadow_bias;\n"
	"uniform vec3 camera_pos_world_space, light_pos_world_space;\n"
	"uniform mat4 model_view_projection, light_model_view_projection;\n"

	"const struct FaceAttribute {\n"
		"vec3 face_normal;\n"
		"ivec2 uv_indices, uv_signs;\n"
	"} face_attributes[5] = FaceAttribute[5](\n" // Flat, right, bottom, left, top
		"FaceAttribute(vec3(0.0f, 1.0f, 0.0f),  ivec2(0, 2), ivec2(1, 1)),\n"
		"FaceAttribute(vec3(1.0f, 0.0f, 0.0f),  ivec2(2, 1), ivec2(-1, -1)),\n"
		"FaceAttribute(vec3(0.0f, 0.0f, 1.0f),  ivec2(0, 1), ivec2(1, -1)),\n"
		"FaceAttribute(vec3(-1.0f, 0.0f, 0.0f), ivec2(2, 1), ivec2(1, -1)),\n"
		"FaceAttribute(vec3(0.0f, 0.0f, -1.0f), ivec2(0, 1), ivec2(-1, -1))\n"
	");\n"

	"void set_normal_and_UV_from_face_id(int face_id_bits) {\n"
		"FaceAttribute face_attribute = face_attributes[face_id_bits];\n"

		"face_normal = face_attribute.face_normal;\n"

		"vec2 UV_xy = face_attribute.uv_signs * vec2(\n"
			"vertex_pos_world_space[face_attribute.uv_indices.x],\n"
			"vertex_pos_world_space[face_attribute.uv_indices.y]\n"
		");\n"

		"UV = vec3(UV_xy, face_info_bits >> 3);\n"
	"}\n"

	"void main(void) {\n"
		"vec4 vertex_pos_world_space_4D = vec4(vertex_pos_world_space, 1.0f);\n"

		"gl_Position = model_view_projection * vertex_pos_world_space_4D;\n"
		"set_normal_and_UV_from_face_id(face_info_bits & 7);\n"
		"light_pos_delta_world_space = light_pos_world_space - vertex_pos_world_space;\n"

		"fragment_pos_light_space = light_model_view_projection * vertex_pos_world_space_4D;\n"
		"fragment_pos_light_space.z += shadow_bias;\n"
	"}\n",

*const sector_fragment_shader =
    "#version 330 core\n"

	"in vec3 UV, face_normal, light_pos_delta_world_space;\n"
	"in vec4 fragment_pos_light_space;\n"

	"out vec3 color;\n"

	"uniform float base_ambient, min_attenuation, attenuation_factor, shadow_umbra_strength;\n"
	"uniform vec3 light_pos;\n"
	"uniform sampler2DShadow shadow_map_sampler;\n"
	"uniform sampler2DArray texture_sampler;\n"

	"float attenuation(void) {\n" // Distance-based lighting
		"float dist_squared = dot(light_pos_delta_world_space, light_pos_delta_world_space);\n"
		"float attenuation_percent = 1.0f / (1.0f + attenuation_factor * dist_squared);\n"
		"return clamp(attenuation_percent, min_attenuation, 1.0f);\n"
	"}\n"

	"float diffuse_and_shadow(void) {\n"
		"float diffuse_amount = dot(normalize(light_pos_delta_world_space), face_normal);\n"
		"diffuse_amount = clamp(diffuse_amount, shadow_umbra_strength, 1.0f);\n"

		"vec3 proj_coords = (fragment_pos_light_space.xyz / fragment_pos_light_space.w) * 0.5f + 0.5f;\n"

		"bool in_shadow = texture(shadow_map_sampler, proj_coords) != 1.0f;\n"
		"return in_shadow ? shadow_umbra_strength : diffuse_amount;\n"
	"}\n"

	"float calculate_light(void) {\n"
		"float light = base_ambient + diffuse_and_shadow() * attenuation();\n"
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
