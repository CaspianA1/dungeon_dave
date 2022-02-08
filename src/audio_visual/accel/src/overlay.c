#include "headers/overlay.h"
#include "texture.c"
#include "utils.c"

// Overlay = weapon, hud, and things like that

// TODO: put these in shaders.c later
const GLchar *const weapon_vertex_shader =
	"#version 330 core\n"
	
	"layout(location = 0) in vec3 vertex_pos_screen_space;\n"
	"layout(location = 1) in vec2 UV;\n"

	"out vec2 fragment_UV;\n"

	"void main(void) {\n"
		"gl_Position = vertex_pos_screen_space;\n"
		"fragment_UV = UV;\n"
	"}\n",

*const weapon_fragment_shader =
	"#version 330 core\n"

	"in vec2 UV;\n"

	"out vec4 color;\n"

	"uniform uint frame_index;\n"
	"uniform sampler2DArray frame_sampler;\n"

	"void main(void) {\n"
		"color = texture(texture_sampler, vec3(UV, frame_index));\n"
	"}\n";

Weapon init_weapon(const GLchar* const spritesheet_path, const GLsizei frames_across,
	const GLsizei frames_down, const GLsizei total_frames) {

	/* It's a bit wasteful to load the surface in init_texture_set
	and here, but this makes the code much more readable */
	SDL_Surface* const peek_surface = init_surface(spritesheet_path);
	const GLsizei peek_size[2] = {peek_surface -> w, peek_surface -> h};
	deinit_surface(peek_surface);

	const Weapon weapon = {
		.texture = init_texture_set(
			TexNonRepeating, 0, 1, peek_size[0], peek_size[1],
			frames_across, frames_down, total_frames
		),
		.curr_frame = 0
	};

	use_texture(weapon.texture, weapon.shader, "frame_sampler", TexNonRepeating, WEAPON_TEXTURE_UNIT);

	return weapon;
}
