#include "buffer_defs.h"
#include "camera.h"

typedef struct {
	const GLuint texture, shader;
	buffer_size_t curr_frame;
	const GLfloat frame_width_over_height, size;
} WeaponSprite;

// Excluded: circular_mapping_from_zero_to_one

WeaponSprite init_weapon_sprite(const GLfloat size, const GLchar* const spritesheet_path,
	const GLsizei frames_across, const GLsizei frames_down, const GLsizei total_frames);

void deinit_weapon_sprite(const WeaponSprite weapon);
void draw_weapon_sprite(const WeaponSprite weapon, const Camera* const camera);
