#ifndef SHADERS_C
#define SHADERS_C

const char* const sector_vertex_shader =
	"#version 330 core\n"

	"layout(location = 0) in vec3 vertex_pos_model_space;\n"
	"layout(location = 1) in vec2 vertex_UV;\n"

	"out vec2 UV;\n"

	"uniform mat4 model_view_projection;\n"

	"void main() {\n"
		"gl_Position = model_view_projection * vec4(vertex_pos_model_space, 1);\n"
		"UV = vertex_UV;\n"
	"}\n",

*const sector_fragment_shader =
    "#version 330 core\n"

	"in vec2 UV;\n"
	"out vec3 color;\n" // For textures with an alpha channel, enable 4 channels

	"uniform sampler2D texture_sampler;\n"

	"void main() {\n"
		"color = texture(texture_sampler, UV).rgb;\n"
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
	");"

	"const vec3 cam_up_world_space = vec3(0.0f, 1.0f, 0.0f);\n"

	"void main() {\n"
		"vec2 vertex_model_space = vertices_model_space[gl_VertexID];\n"

		"vec3 vertex_world_space = billboard_center_world_space \n"
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

	"layout(location = 0) in vec3 vertex_pos_model_space;\n"

	"out vec3 UV_3D;\n"

	"uniform mat4 view_projection;\n"

	"void main() {\n"
		"gl_Position = (view_projection * vec4(vertex_pos_model_space, 1.0)).xyww;\n"
		"UV_3D = vertex_pos_model_space;\n"
	"}\n",

*const skybox_fragment_shader =
    "#version 330 core\n"

	"in vec3 UV_3D;\n"

	"out vec4 color;\n"

	"uniform samplerCube cubemap_sampler;\n"

	"void main() {\n"
		"color = texture(cubemap_sampler, UV_3D);\n"
	"}\n";

#endif
