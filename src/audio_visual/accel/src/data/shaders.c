#ifndef SHADERS_C
#define SHADERS_C

const char* const sector_vertex_shader =
	"#version 330 core\n"
	"#define max_world_height 255.0f\n"

	"layout(location = 0) in vec3 vertex_pos_world_space;\n"
	"layout(location = 1) in int face_info_bits;\n"

	"out vec3 UV, face_normal, pos_delta_world_space;\n"

	"uniform vec3 light_pos_world_space;\n"
	"uniform mat4 model_view_projection;\n"

	"const ivec2 pos_indices_for_UV[3] = ivec2[3](\n"
		"ivec2(0, 2), ivec2(2, 1), ivec2(0, 1)\n" // Flat, NS, EW
	");\n"

	"const vec3 face_normals[5] = vec3[5](\n"
		"vec3(0.0f, 1.0f, 0.0f), vec3(1.0f, 0.0f, 0.0f),\n" // Flat, right, bottom, left, top
		"vec3(0.0f, 0.0f, 1.0f), vec3(-1.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, -1.0f)\n"
	");\n"

	"void set_UV_from_face_id(int face_id_bits) {\n"
		"ivec2 pos_UV_indices = pos_indices_for_UV[face_id_bits & 3];\n"  // `& 3` extracts the first 2 bits
		"int face_is_left_or_bottom = int(face_id_bits == 2 || face_id_bits == 5);\n" // 1 = true, 0 = false
		"int UV_sign_x = -((face_is_left_or_bottom << 1) - 1);\n" // -((x << 1) - 1) maps 1 to -1 and 0 to 1
		"vec3 pos_reversed = max_world_height - vertex_pos_world_space;\n"

		"UV = vec3(\n"
			"pos_reversed[pos_UV_indices[0]] * UV_sign_x,\n"
			"pos_reversed[pos_UV_indices[1]],\n"
			"face_info_bits >> 3);\n" // `>> 3` puts texture id bits into start of number
	"}\n"

	/* In order to map {0 1 2 5 6} to {0 1 2 3 4}, do this:
		- If the bits equal 5 or 6, the 3rd bit will be set. By and-ing the bits
		with 0b100 (which is 4), `X` will equal 4 if the bits equalled 5 or 6.
		- Then, the normal ID will equal `face_id_bits - (X >> 1)`, because the right shift
		will divide `X` by 2, and therefore map 5 and 6 to 3 and 4. 0 through 3 will stay the same. */

	"void set_normal_from_face_id(int face_id_bits) {\n"
		"int normal_id_subtrahend = (face_id_bits & 4) >> 1;\n"
		"face_normal = face_normals[face_id_bits - normal_id_subtrahend];\n"
	"}\n"

	"void main(void) {\n"
		"gl_Position = model_view_projection * vec4(vertex_pos_world_space, 1.0f);\n"
		"int face_id_bits = face_info_bits & 7;\n" // 0 = flat, 1 = right, 2 = bottom, 5 = left, 6 = top

		"set_UV_from_face_id(face_id_bits);\n"
		"set_normal_from_face_id(face_id_bits);\n"

		"pos_delta_world_space = light_pos_world_space - vertex_pos_world_space;\n"
	"}\n",

*const sector_fragment_shader =
    "#version 330 core\n"

	"in vec3 UV, face_normal, pos_delta_world_space;\n"

	"out vec3 color;\n"

	"uniform float ambient_strength, diffuse_strength;\n"
	"uniform sampler2DArray texture_sampler;\n"

	"float diffuse(void) {\n" // Faces get darker as the view angle from it gets steeper
		"vec3 light_dir = normalize(pos_delta_world_space);\n"
		"return dot(light_dir, face_normal) * diffuse_strength;\n"
	"}\n"

	"float attenuation(void) {\n" // Distance-based lighting
		"float dist_squared = dot(pos_delta_world_space, pos_delta_world_space);\n"
		"return 1.0f / (0.9f + 0.005f * dist_squared);\n"
	"}\n"

	"void main(void) {\n"
		"float light = (ambient_strength + diffuse()) * attenuation();\n"
		"color = texture(texture_sampler, UV).rgb * min(light, 1.0f);\n"
	"}\n",

*const sector_lighting_vertex_shader =
    "#version 330 core\n"

	"layout(location = 0) in vec3 vertex_pos_world_space;\n"
	"layout(location = 1) in vec2 vertex_UV;\n"

	"out vec2 UV;\n"
	"out vec3 pos_delta_world_space;\n"

	"uniform vec3 camera_pos_world_space;\n"
	"uniform mat4 model_view_projection;\n"

	"void main(void) {\n"
		"gl_Position = model_view_projection * vec4(vertex_pos_world_space, 1.0f);\n"
		"UV = vertex_UV;\n"
		"pos_delta_world_space = camera_pos_world_space - vertex_pos_world_space;\n"
	"}\n",

*const sector_lighting_fragment_shader =
    "#version 330 core\n"

	"in vec2 UV;\n"
	"in vec3 pos_delta_world_space;\n"

	"out vec3 color;\n"

	"uniform sampler2D texture_sampler;\n"

	"const float min_light = 0.1f, max_light = 1.0f, intensity_factor = 50.0f;\n"

	"void main(void) {\n" // dist_squared is distance squared from fragment
		"float dist_squared = dot(pos_delta_world_space, pos_delta_world_space);\n"
		"float light_intensity = clamp(intensity_factor / dist_squared, min_light, max_light);\n"

		"color = light_intensity * texture(texture_sampler, UV).rgb;\n"
	"}\n",

*const billboard_vertex_shader =
	"#version 330 core\n"

	"out vec2 UV;\n"

	"uniform vec2 billboard_size_world_space, right_xz_world_space;\n"
	"uniform vec3 billboard_center_world_space;\n"
	"uniform mat4 view_projection;\n"

	"const vec2 vertices_model_space[4] = vec2[4](\n"
		"vec2(-0.5f, -0.5f), vec2(0.5f, -0.5f),\n"
		"vec2(-0.5f, 0.5f), vec2(0.5f, 0.5f)\n"
	");\n"

	"const vec3 up_world_space = vec3(0.0f, 1.0f, 0.0f);\n"

	"void main(void) {\n"
		"vec2 vertex_model_space = vertices_model_space[gl_VertexID];\n"
		"vec2 corner_world_space = vertex_model_space * billboard_size_world_space;\n"

		"vec3 vertex_world_space = billboard_center_world_space +\n"
			"corner_world_space.x * vec3(right_xz_world_space, 0.0f).xzy\n"
			"+ corner_world_space.y * up_world_space;\n"

		"gl_Position = view_projection * vec4(vertex_world_space, 1.0f);\n"
		"UV = vec2(vertex_model_space.x, -vertex_model_space.y) + 0.5f;\n"
	"}\n",

*const billboard_fragment_shader =
    "#version 330 core\n"

	"in vec2 UV;\n"

	"out vec4 color;\n"

	"uniform sampler2D texture_sampler;\n"

	"void main(void) {\n"
		"color = texture(texture_sampler, UV);\n"
	"}\n",

*const skybox_vertex_shader =
	"#version 330 core\n"

	"layout(location = 0) in vec3 vertex_pos_world_space;\n"

	"out vec3 UV_3D;\n"

	"uniform mat4 view_projection;\n"

	"void main(void) {\n"
		"gl_Position = (view_projection * vec4(vertex_pos_world_space, 1.0f)).xyww;\n"
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

	"layout(location = 0) in vec2 vertex_pos;\n"

	"out vec2 UV, angles;\n"

	"uniform float time;\n" // In seconds

	"void main(void) {\n"
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
