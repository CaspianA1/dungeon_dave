#include "texture.h"

typedef struct {
	GLuint texture, shader;
	buffer_size_t curr_frame;
} Weapon;

Weapon init_weapon(const GLchar* const spritesheet_path, const GLsizei frames_across,
	const GLsizei frames_down, const GLsizei total_frames);

void deinit_weapon(const Weapon weapon);
void draw_weapon(const Weapon weapon);
