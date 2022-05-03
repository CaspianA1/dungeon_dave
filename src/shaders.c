#ifndef SHADERS_C
#define SHADERS_C

#include "headers/buffer_defs.h"
#include "headers/shaders.h"

const GLchar *const sector_vertex_shader =
	"#version 330 core\n"

	"layout(location = 0) in vec3 vertex_pos_world_space;\n"
	"layout(location = 1) in uint face_info_bits;\n"

	"flat out uint face_id;\n"

	"out vec3 UV, fragment_pos_light_space, camera_pos_delta_world_space;\n"

	"uniform vec3 camera_pos_world_space;\n"
	"uniform mat4 model_view_projection, light_model_view_projection;\n"

	"const struct FaceAttribute {\n"
		"ivec2 uv_indices, uv_signs;\n"
	"} face_attributes[5] = FaceAttribute[5](\n" // Flat, right, bottom, left, top
		"FaceAttribute(ivec2(0, 2), ivec2(1, 1)),\n"
		"FaceAttribute(ivec2(2, 1), ivec2(-1, -1)),\n"
		"FaceAttribute(ivec2(0, 1), ivec2(1, -1)),\n"
		"FaceAttribute(ivec2(2, 1), ivec2(1, -1)),\n"
		"FaceAttribute(ivec2(0, 1), ivec2(-1, -1))\n"
	");\n"

	"void main(void) {\n"
		////////// Setting face_normal and UV

		"face_id = face_info_bits & 7u;\n"
		"FaceAttribute face_attribute = face_attributes[face_id];\n"

		"vec2 UV_xy = face_attribute.uv_signs * vec2(\n"
			"vertex_pos_world_space[face_attribute.uv_indices.x],\n"
			"vertex_pos_world_space[face_attribute.uv_indices.y]\n"
		");\n"

		"UV = vec3(UV_xy, face_info_bits >> 3u);\n"

		////////// Setting camera_pos_delta_world_space, fragment_pos_light_space, and gl_Position

		"vec4 vertex_pos_world_space_4D = vec4(vertex_pos_world_space, 1.0f);\n"

		"camera_pos_delta_world_space = camera_pos_world_space - vertex_pos_world_space;\n"
		"fragment_pos_light_space = vec3(light_model_view_projection * vertex_pos_world_space_4D) * 0.5f + 0.5f;\n"

		"gl_Position = model_view_projection * vertex_pos_world_space_4D;\n"
	"}\n",

*const sector_fragment_shader =
	"#version 330 core\n"

	"flat in uint face_id;\n"
	"in vec3 UV, fragment_pos_light_space, camera_pos_delta_world_space;\n"

	"out vec3 color;\n"

	"uniform bool enable_tone_mapping;\n"
	"uniform int pcf_radius;\n"

	"uniform float\n"
		"esm_constant, ambient, diffuse_strength,\n"
		"specular_strength, exposure, noise_granularity;\n"

	"uniform vec2 specular_exponent_domain, one_over_screen_size;\n"
	"uniform vec3 dir_to_light, light_color;\n" // `dir_to_light` is the direction pointing to the light source

	"uniform sampler2D shadow_map_sampler;\n"
	"uniform sampler2DArray texture_sampler, normal_map_sampler;\n"

	"float diffuse(vec3 fragment_normal) {\n"
		"float diffuse_amount = dot(fragment_normal, dir_to_light);\n"
		"return diffuse_strength * max(diffuse_amount, 0.0f);\n"
	"}\n"

	"float specular(vec3 texture_color, vec3 fragment_normal) {\n"
		/* Brighter texture colors have more specularity, and stronger highlights.
		Also, the specular calculation uses Blinn-Phong, rather than just Phong. */

		"vec3 view_dir = normalize(camera_pos_delta_world_space);\n"
		"vec3 halfway_dir = normalize(dir_to_light + view_dir);\n"
		"float cos_angle_of_incidence = max(dot(fragment_normal, halfway_dir), 0.0f);\n"

		//////////

		"const float one_third = 1.0f / 3.0f;\n"
		"float texture_color_strength = (texture_color.r + texture_color.g + texture_color.b) * one_third;\n"

		"float specular_exponent = mix(specular_exponent_domain.x, specular_exponent_domain.y, texture_color_strength);\n"
		"return specular_strength * texture_color_strength * pow(cos_angle_of_incidence, specular_exponent);\n"
	"}\n"

	"float shadow(void) {\n"
		/* Notes:
		- Lighter shadow bases are simply an artifact of ESM
		- A higher exponent means more shadow acne, but less light bleeding */

		"vec2\n"
			"shadow_map_UV = fragment_pos_light_space.xy,\n"
			"texel_size = 1.0f / textureSize(shadow_map_sampler, 0);\n"

		"float average_occluder_depth = 0.0f;\n"

		"for (int y = -pcf_radius; y <= pcf_radius; y++) {\n"
			"for (int x = -pcf_radius; x <= pcf_radius; x++) {\n"
				"vec2 sample_UV = shadow_map_UV + texel_size * vec2(x, y);\n"
				"average_occluder_depth += texture(shadow_map_sampler, sample_UV).r;\n"
			"}\n"
		"}\n"

		"int samples_across = (pcf_radius << 1) + 1;\n"
		"average_occluder_depth *= 1.0f / (samples_across * samples_across);\n"

		//////////

		"float occluder_receiver_diff = fragment_pos_light_space.z - average_occluder_depth;\n"
		"float in_light_percentage = exp(-esm_constant * occluder_receiver_diff);\n"
		"return clamp(in_light_percentage, 0.0f, 1.0f);\n"
	"}\n"

	"vec3 calculate_light(vec3 texture_color, vec3 fragment_normal) {\n"
		"float non_ambient = diffuse(fragment_normal) + specular(texture_color, fragment_normal);\n"
		"float shadowed_non_ambient = non_ambient * shadow();\n"
		"float light_strength = ambient + shadowed_non_ambient;\n"
		"return light_strength * light_color * texture_color;\n"
	"}\n"

	"vec3 get_fragment_normal(void) {\n"
		// `t` = tangent space normal. Normalized b/c linear filtering may unnormalize it.
		"vec3 t = normalize(texture(normal_map_sampler, UV).rgb * 2.0f - 1.0f);\n"

		// No matrix multiplication here! :)
		"vec3 rotated_vectors[5] = vec3[5](\n"
			"vec3(t.xz, -t.y),\n" // Flat
			"vec3(t.zy, -t.x),\n" // Right
			"t,\n" // Bottom (equal to tangent space)
			"vec3(-t.z, t.yx),\n" // Left
			"vec3(-t.x, t.y, -t.z)\n" // Top (opposite of tangent space)
		");\n"

		"return rotated_vectors[face_id];\n"
	"}\n"

	"vec3 postprocess_light(vec3 color) {\n"
		// HDR through tone mapping
		"vec3 tone_mapped_color = vec3(1.0f) - exp(-color * exposure);\n"
		"color = mix(color, tone_mapped_color, float(enable_tone_mapping));\n"

		// Noise is added to remove color banding
		"vec2 screen_fragment_pos = gl_FragCoord.xy * one_over_screen_size;\n"
		"float random_value = fract(sin(dot(screen_fragment_pos, vec2(12.9898f, 78.233f))) * 43758.5453f);\n"
		"return color + mix(-noise_granularity, noise_granularity, random_value);\n"
	"}\n"

	"void main(void) {\n"
		"color = calculate_light(texture(texture_sampler, UV).rgb, get_fragment_normal());\n"
		"color = postprocess_light(color);\n"
	"}\n",

*const billboard_vertex_shader =
	"#version 330 core\n"

	"layout(location = 0) in uint texture_id;\n"
	"layout(location = 1) in vec2 billboard_size_world_space;\n"
	"layout(location = 2) in vec3 billboard_center_world_space;\n"

	"out vec3 UV, fragment_pos_light_space;\n"

	"uniform vec2 right_xz_world_space;\n"
	"uniform mat4 model_view_projection, light_model_view_projection;\n"

	"const vec2 vertices_model_space[4] = vec2[4](\n"
		"vec2(-0.5f, -0.5f), vec2(0.5f, -0.5f),\n"
		"vec2(-0.5f, 0.5f), vec2(0.5f, 0.5f)\n"
	");\n"

	"void main(void) {\n"
		"vec2 vertex_pos_model_space = vertices_model_space[gl_VertexID];\n"
		"vec2 upscaled_vertex_pos_world_space = vertex_pos_model_space * billboard_size_world_space;\n"

		"vec3 vertex_pos_world_space = billboard_center_world_space\n"
			"+ upscaled_vertex_pos_world_space.x * vec3(right_xz_world_space, 0.0f).xzy\n"
			"+ vec3(0.0f, upscaled_vertex_pos_world_space.y, 0.0f);\n"

		"vec4 vertex_pos_world_space_4D = vec4(vertex_pos_world_space, 1.0f);\n"
		"gl_Position = model_view_projection * vertex_pos_world_space_4D;\n"
		"fragment_pos_light_space = vec3(light_model_view_projection * vertex_pos_world_space_4D) * 0.5f + 0.5f;\n"

		"UV.xy = vec2(vertex_pos_model_space.x, -vertex_pos_model_space.y) + 0.5f;\n"
		"UV.z = texture_id;\n"
	"}\n",

*const billboard_fragment_shader =
	"#version 330 core\n"

	"in vec3 UV, fragment_pos_light_space;\n"

	"out vec4 color;\n"

	"uniform int pcf_radius;\n"
	"uniform float ambient, esm_constant;\n"
	"uniform sampler2D shadow_map_sampler;\n"
	"uniform sampler2DArray texture_sampler;\n"

	"float shadow(void) {\n" // TODO: share this function between the sector and billboard fragment shaders
		"vec2\n"
			"shadow_map_UV = fragment_pos_light_space.xy,\n"
			"texel_size = 1.0f / textureSize(shadow_map_sampler, 0);\n"

		"float average_occluder_depth = 0.0f;\n"

		"for (int y = -pcf_radius; y <= pcf_radius; y++) {\n"
			"for (int x = -pcf_radius; x <= pcf_radius; x++) {\n"
				"vec2 sample_UV = shadow_map_UV + texel_size * vec2(x, y);\n"
				"average_occluder_depth += texture(shadow_map_sampler, sample_UV).r;\n"
			"}\n"
		"}\n"

		"int samples_across = (pcf_radius << 1) + 1;\n"
		"average_occluder_depth *= 1.0f / (samples_across * samples_across);\n"

		"float result = exp(esm_constant * (average_occluder_depth - fragment_pos_light_space.z));\n"
		"return clamp(result, 0.0f, 1.0f);\n"
	"}\n"

	"void main(void) {\n"
		"color = texture(texture_sampler, UV);\n"
		"color.rgb *= mix(ambient, 1.0f, shadow());\n"
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

*const weapon_vertex_shader =
	"#version 330 core\n"

	"uniform float frame_width_over_height, weapon_size_screen_space, inverse_screen_aspect_ratio;\n"
	"uniform vec2 pace;\n"

	"out vec2 fragment_UV;\n"

	"const vec2 screen_corners[4] = vec2[4] (\n"
		"vec2(-1.0f, -1.0f), vec2(1.0f, -1.0f),\n"
		"vec2(-1.0f, 1.0f), vec2(1.0f, 1.0f)\n"
	");\n"

	"void main(void) {\n"
		"vec2 screen_corner = screen_corners[gl_VertexID];\n"

		"vec2 weapon_corner = screen_corner * weapon_size_screen_space;\n"
		"weapon_corner.x *= frame_width_over_height * inverse_screen_aspect_ratio;\n"

		"weapon_corner.y += weapon_size_screen_space - 1.0f;\n" // Makes the weapon touch the bottom of the screen
		"weapon_corner += pace;\n"

		"gl_Position = vec4(weapon_corner, 0.0f, 1.0f);\n"
		"fragment_UV = vec2(screen_corner.x, -screen_corner.y) * 0.5f + 0.5f;\n"
	"}\n",

*const weapon_fragment_shader =
	"#version 330 core\n"

	"in vec2 fragment_UV;\n"

	"out vec4 color;\n"

	"uniform uint frame_index;\n"
	"uniform sampler2DArray frame_sampler;\n"

	"void main(void) {\n"
		"color = texture(frame_sampler, vec3(fragment_UV, frame_index));\n"
	"}\n",

*const depth_vertex_shader =
	"#version 330 core\n"

	"layout(location = 0) in vec3 vertex_pos_world_space;\n"

	"uniform mat4 light_model_view_projection;\n"

	"void main(void) {\n"
		"gl_Position = light_model_view_projection * vec4(vertex_pos_world_space, 1.0f);\n"
	"}\n",

*const depth_fragment_shader =
	"#version 330 core\n"

	"void main(void) {}\n";

#endif
