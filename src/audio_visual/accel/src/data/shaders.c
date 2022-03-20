#ifndef SHADERS_C
#define SHADERS_C

#include "../headers/utils.h"

const GLchar *const sector_vertex_shader =
	"#version 330 core\n"

	"layout(location = 0) in vec3 vertex_pos_world_space;\n"
	"layout(location = 1) in int face_info_bits;\n"

	"out vec3 UV, face_normal, fragment_pos_light_space, camera_pos_delta_world_space;\n"

	"uniform vec3 camera_pos_world_space;\n"
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

	"void main(void) {\n"
		////////// Setting face_normal and UV

		"FaceAttribute face_attribute = face_attributes[face_info_bits & 7];\n"
		"face_normal = face_attribute.face_normal;\n"

		"vec2 UV_xy = face_attribute.uv_signs * vec2(\n"
			"vertex_pos_world_space[face_attribute.uv_indices.x],\n"
			"vertex_pos_world_space[face_attribute.uv_indices.y]\n"
		");\n"

		"UV = vec3(UV_xy, face_info_bits >> 3);\n"

		////////// Setting camera_pos_delta_world_space, fragment_pos_light_space, and gl_Position

		"vec4 vertex_pos_world_space_4D = vec4(vertex_pos_world_space, 1.0f);\n"

		"camera_pos_delta_world_space = camera_pos_world_space - vertex_pos_world_space;\n"
		"fragment_pos_light_space = vec3(light_model_view_projection * vertex_pos_world_space_4D);\n"

		"gl_Position = model_view_projection * vertex_pos_world_space_4D;\n"
	"}\n",

*const sector_fragment_shader =
	"#version 330 core\n"

	"in vec3 UV, face_normal, fragment_pos_light_space, camera_pos_delta_world_space;\n"

	"out vec3 color;\n"

	"uniform float overall_light_strength, ambient, shininess, umbra_strength_factor;\n"
	"uniform vec2 warp_exps;\n"
	"uniform vec3 inv_light_dir;\n"

	"uniform sampler2D shadow_map_sampler;\n"
	"uniform sampler2DArray texture_sampler;\n"

	"float diffuse(void) {\n"
		"float diffuse_amount = dot(inv_light_dir, face_normal);\n"
		"return max(diffuse_amount, 0.0f);\n"
	"}\n"

	"float specular(void) {\n" // Uses Blinn-Phong specular, rather than Phong specular
		"vec3 halfway_dir = normalize(inv_light_dir + normalize(camera_pos_delta_world_space));\n"
		"return pow(max(dot(face_normal, halfway_dir), 0.0f), shininess);\n"
	"}\n"

	"vec2 warp_depth(float depth) {\n"
		"return vec2(exp(warp_exps.x * depth), -exp(-warp_exps.y * depth));\n"
	"}\n"

	"float chebyshev(float min_variance, float depth, vec2 moments) {\n"
		"float\n"
			"d = depth - moments.x,\n" // `d` = distance between receiver (depth) and occluder (moments.x)
			"variance = max(moments.y - moments.x * moments.x, min_variance);\n"

		// TODO: see if clamping variance on the lower bound actually makes a difference

		"float p_max = variance / (variance + (d * d));\n"
		"return (d < 0.0f) ? 1.0f : p_max;\n"
	"}\n"

	"float one_minus_shadow_percent(void) {\n" // Gives the percent of area in shadow via EVSM
		"vec3 proj_coords = fragment_pos_light_space * 0.5f + 0.5f;\n"
		"vec4 moments = texture(shadow_map_sampler, proj_coords.xy);\n"

		"vec2\n"
			"w_depth = warp_depth(proj_coords.z),\n"
			"pos_moments = moments.xz, neg_moments = moments.yw;\n"

		"vec2 depth_scale = umbra_strength_factor * warp_exps * w_depth;\n"
		"vec2 min_variance = depth_scale * depth_scale;\n"

		"float\n"
			"pos_result = chebyshev(min_variance.x, w_depth.x, pos_moments),\n"
			"neg_result = chebyshev(min_variance.y, w_depth.y, neg_moments);\n"

		"return min(pos_result, neg_result);\n"
	"}\n"

	"float calculate_light(void) {\n"
		"float diffuse_amount = diffuse();\n"

		 // Modulating specular by how much the face is facing the light source
		"float non_ambient = diffuse_amount + specular() * diffuse_amount;\n"

		"float light = ambient + non_ambient * one_minus_shadow_percent();\n"
		"return min(light * overall_light_strength, 1.0f);\n"
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
	"}\n";

#endif
