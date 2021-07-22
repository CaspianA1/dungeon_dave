#include "global_defs.h"
#include "../audio_visual/sound.c"
#include "../audio_visual/overlay/overlay.h"
#include "../combat/combat.h"
#include "global_types.h"

#include "utils.c"

#include "../audio_visual/lighting.c"
#include "settings.c"
#include "screen.c"
#include "dda.c"
#include "statemap.c"

#include "../audio_visual/overlay/mipmap.c"
#include "../audio_visual/overlay/sprite.c"
#include "../audio_visual/overlay/animation.c"
#include "../audio_visual/overlay/render_overlay.c"

#include "level.c"
#include "input.c"

#include "../audio_visual/gui/gui.h"
#include "../audio_visual/gui/gui.c"
#include "../audio_visual/gui/title_screen.c"
#include "../audio_visual/gui/hud.c"
#include "../audio_visual/gui/option_screen.c"

#include "../combat/bfs_types.c"
#include "../combat/bfs.c"
#include "../combat/navigator.c"
#include "../combat/enemy.c"
#include "../combat/weapon.c"

#include "../audio_visual/floorcast.c"
#include "../audio_visual/raycast.c"

#include "../levels/level_1.c"
#include "../levels/debug_level.c"
#include "../levels/palace.c"
#include "../levels/red_room.c"

/*
itinerary:
fast floor shading
non-clipping pathfinding
*/

// drawing order: skybox, walls, generic billboards, weapon, floor, minimap, crosshair
int main(void) {
	Player player;
	Weapon weapon;
	load_all_defaults(load_palace, &player, &weapon);

	if (display_title_screen() == Exit)
		deinit_all(player, weapon);

	play_sound(current_level.background_sound, 1);

	p = init_psprite("assets/walls/dune.bmp");

	while (1) {
		const Uint32 before = SDL_GetTicks();
		if (keys[SDL_SCANCODE_C]) DEBUG_VEC(player.pos);

		const InputStatus input_status = handle_input(&player, 0);
		if (input_status == Exit) deinit_all(player, weapon);

		update_screen_dimensions(&player.y_pitch, player.mouse_pos.y);
		clear_statemap(occluded_pixels);

		const double wall_y_shift = settings.half_screen_height + player.y_pitch + player.pace.screen_offset;

		prepare_for_drawing();
		draw_skybox(player.angle, wall_y_shift);
		// draw_colored_floor(wall_y_shift);

		#ifndef PLANAR_MODE
		const double full_jump_height = player.jump.height * settings.screen_height;
		raycast(player, wall_y_shift, full_jump_height);
		draw_generic_billboards(player, wall_y_shift);
		update_all_enemies(player);
		use_weapon_if_needed(&weapon, player, input_status);
		#else
		fill_val_buffers_for_planar_mode(player.angle);
		#endif

		fast_affine_floor(player.pos, player.jump.height, player.pace.screen_offset, wall_y_shift, player.y_pitch);

		refresh(player.tilt, player.pos, wall_y_shift);
		tick_delay(before);
	}
}
