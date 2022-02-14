#include "buffer_defs.h"
#include "camera.h"

typedef struct {
	const GLuint texture, shader;
	const GLfloat frame_width_over_height, size;
	buffer_size_t curr_frame;
} Weapon;

// Excluded: circular_mapping_from_zero_to_one

Weapon init_weapon(const GLfloat size, const GLchar* const spritesheet_path,
	const GLsizei frames_across, const GLsizei frames_down, const GLsizei total_frames);

void deinit_weapon(const Weapon weapon);
void draw_weapon(const Weapon weapon, const Camera* const camera);
