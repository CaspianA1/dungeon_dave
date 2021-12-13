#ifndef SHADERS_C
#define SHADERS_C

const char* const sector_vertex_shader =
	"#version 330 core\n"
	"#define max_world_height 255.0f\n"
	"#define darkest_light 0.6f\n"
	"#define light_step 0.2f\n" // From the darkest side, this is the step amount
	"#define sign_of_cond(cond) ((int(cond) << 1) - 1)\n" // 1 -> 1, and 0 -> -1

	"layout(location = 0) in vec3 vertex_pos_world_space;\n"
	"layout(location = 1) in int face_info;\n"

	"flat out int texture_id;\n"
	"out vec2 UV;\n"
	"out vec3 fragment_pos_world_space, face_normal;\n"

	"uniform mat4 model_view_projection;\n"

	"const ivec2 pos_indices_for_UV[3] = ivec2[3](\n"
		"ivec2(0, 2), ivec2(2, 1), ivec2(0, 1)\n" // Flat, NS, EW
	");\n"

	"vec3 get_normal(int first_three_bits) {\n"
		"switch (first_three_bits) {\n"
			"case 0: return vec3(0.0f, 1.0f, 0.0f);\n"
			"case 1: return vec3(1.0f, 0.0f, 0.0f);\n"
			"case 5: return vec3(-1.0f, 0.0f, 0.0f);\n"
			"case 2: return vec3(0.0f, 0.0f, 1.0f);\n"
			"case 6: return vec3(0.0f, 0.0f, -1.0f);\n"
			"default: return vec3(0.0f, 0.0f, 0.0f);\n"

		"}\n"

	"}\n"

	"void main() {\n"
		"gl_Position = model_view_projection * vec4(vertex_pos_world_space, 1.0f);\n"

		"texture_id = face_info >> 3;\n" // `>> 3` shifts upper 5 bits of texture id to the beginning
		"int first_three_bits = face_info & 7;\n"

		"ivec2 index_for_UV = pos_indices_for_UV[face_info & 3];\n"  // `& 3` gets first 2 bits
		"int UV_sign = -sign_of_cond(first_three_bits == 2 || first_three_bits == 5);\n" // Negative if face side is left or bottom

		"vec3 pos_reversed = max_world_height - vertex_pos_world_space;\n"
		"UV = vec2(pos_reversed[index_for_UV[0]] * UV_sign, pos_reversed[index_for_UV[1]]);\n"

		"fragment_pos_world_space = vertex_pos_world_space;\n"
		"face_normal = get_normal(first_three_bits);\n"
	"}\n",

*const sector_fragment_shader =
    "#version 330 core\n"

	"flat in int texture_id;"
	"in vec2 UV;\n"
	"in vec3 fragment_pos_world_space, face_normal;\n"

	"out vec3 color;\n"

	"uniform float ambient;\n"
	"uniform vec3 light_pos_world_space;\n"
	"uniform sampler2DArray texture_sampler;\n"

	"float diffuse(void) {\n"
		"vec3 light_vector = normalize(light_pos_world_space - fragment_pos_world_space);\n"
		"return dot(light_vector, face_normal);\n"
	"}\n"

	"void main() {\n"
		"float light = min(diffuse() + ambient, 1.0f);\n"
		"color = texture(texture_sampler, vec3(UV, texture_id)).rgb * light;\n"
	"}\n",

*const sector_lighting_vertex_shader =
    "#version 330 core\n"

	"layout(location = 0) in vec3 vertex_pos_world_space;\n"
	"layout(location = 1) in vec2 vertex_UV;\n"

	"out vec2 UV;\n"
	"out vec3 pos_delta_world_space;\n"

	"uniform vec3 camera_pos_world_space;\n"
	"uniform mat4 model_view_projection;\n"

	"void main() {\n"
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

	"void main() {\n" // dist_squared is distance squared from fragment
		"float dist_squared = dot(pos_delta_world_space, pos_delta_world_space);\n"
		"float light_intensity = clamp(intensity_factor / dist_squared, min_light, max_light);\n"

		"color = light_intensity * texture(texture_sampler, UV).rgb;\n"
	"}\n",

*const billboard_vertex_shader =
	"#version 330 core\n"

	"out vec2 UV;\n"

	"uniform vec2 billboard_size_world_space, cam_right_xz_world_space;\n"
	"uniform vec3 billboard_center_world_space;\n"
	"uniform mat4 view_projection;\n"

	"const vec2 vertices_model_space[4] = vec2[4](\n"
		"vec2(-0.5f, -0.5f), vec2(0.5f, -0.5f),\n"
		"vec2(-0.5f, 0.5f), vec2(0.5f, 0.5f)\n"
	");\n"

	"const vec3 cam_up_world_space = vec3(0.0f, 1.0f, 0.0f);\n"

	"void main() {\n"
		"vec2 vertex_model_space = vertices_model_space[gl_VertexID];\n"

		"vec3 vertex_world_space = billboard_center_world_space\n"
			"+ vec3(cam_right_xz_world_space, 0.0f).xzy * vertex_model_space.x * billboard_size_world_space.x\n"
			"+ cam_up_world_space * vertex_model_space.y * billboard_size_world_space.y;\n"

		"gl_Position = view_projection * vec4(vertex_world_space, 1.0f);\n"
		"UV = vec2(vertex_model_space.x, -vertex_model_space.y) + 0.5f;\n"
	"}\n",

*const billboard_fragment_shader =
    "#version 330 core\n"

	"in vec2 UV;\n"

	"out vec4 color;\n"

	"uniform sampler2D texture_sampler;\n"

	"void main() {\n"
		"color = texture(texture_sampler, UV);\n"
	"}\n",

*const skybox_vertex_shader =
	"#version 330 core\n"

	"layout(location = 0) in vec3 vertex_pos_world_space;\n"

	"out vec3 UV_3D;\n"

	"uniform mat4 view_projection;\n"

	"void main() {\n"
		"gl_Position = (view_projection * vec4(vertex_pos_world_space, 1.0f)).xyww;\n"
		"UV_3D = vertex_pos_world_space;\n"
		"UV_3D.x = -UV_3D.x;\n" // Without this, the X component of the UV is reversed
	"}\n",

*const skybox_fragment_shader =
    "#version 330 core\n"

	"in vec3 UV_3D;\n"

	"out vec3 color;\n"

	"uniform samplerCube texture_sampler;\n"

	"void main() {\n"
		"color = texture(texture_sampler, UV_3D).rgb;\n"
	"}\n",

*const water_vertex_shader =
	"#version 330 core\n"
	"#define sin_cycles 6.28f\n"
	"#define speed 2.0f\n"

	"layout(location = 0) in vec2 vertex_pos;\n"

	"out vec2 UV, angles;\n"

	"uniform float time;\n" // In seconds

	"void main() {\n"
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

	"void main() {\n"
		"vec2 wavy_UV = UV + sin(angles) * dist_mag;\n"
		"color = texture(texture_sampler, wavy_UV).rgb;\n"
	"}\n";

#endif
