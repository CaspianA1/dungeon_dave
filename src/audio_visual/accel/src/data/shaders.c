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

	"out float light;\n"
	"out vec2 UV;\n"

	"uniform mat4 model_view_projection;\n"

	"const ivec2 pos_indices_for_UV[3] = ivec2[3](\n"
		"ivec2(0, 2), ivec2(2, 1), ivec2(0, 1)\n" // Flat, NS, EW
	");\n"

	"void main() {\n"
		"gl_Position = model_view_projection * vec4(vertex_pos_world_space, 1);\n"

		"int face_type = face_info & 3;"
		"ivec2 index_for_UV = pos_indices_for_UV[face_type];\n" // Masking with 3 gets first 2 bits (face type)
		"int UV_sign = -sign_of_cond(face_info == 2 || face_info == 5);\n" // Negative if face side is left or bottom

		"vec3 pos_reversed = max_world_height - vertex_pos_world_space;\n"
		"UV = vec2(pos_reversed[index_for_UV[0]] * UV_sign, pos_reversed[index_for_UV[1]]);\n"

		// Top = 1.0f, top or left = 0.8f, bottom or right = 0.6f
		"bool side = (face_info & 4) == 0, flat_face = face_info == 0;\n" // `side` means flat, top or left
		"light = darkest_light + (float(side) * light_step) + (float(flat_face) * light_step);\n"
	"}\n",

*const sector_fragment_shader =
    "#version 330 core\n"

	"in float light;\n"
	"in vec2 UV;\n"

	"out vec3 color;\n"

	"uniform sampler2DArray texture_sampler;\n"

	"void main() {\n"
		"color = texture(texture_sampler, vec3(UV, 0.0f)).rgb * light;\n"
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
		"gl_Position = model_view_projection * vec4(vertex_pos_world_space, 1);\n"
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
		"gl_Position = (view_projection * vec4(vertex_pos_world_space, 1.0)).xyww;\n"
		"UV_3D = vertex_pos_world_space;\n"
	"}\n",

*const skybox_fragment_shader =
    "#version 330 core\n"

	"in vec3 UV_3D;\n"

	"out vec4 color;\n"

	"uniform samplerCube texture_sampler;\n"

	"void main() {\n"
		"color = texture(texture_sampler, UV_3D);\n"
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
