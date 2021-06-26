#include "global_defs.h"
#include "../audio_visual/sound.c"
#include "../audio_visual/overlay/overlay.h"
#include "../combat/combat.h"
#include "global_types.h"

#include "utils.c"

#include "../audio_visual/lighting.c"
#include "settings.c"
#include "screen.c"

#include "../audio_visual/overlay/sprite.c"
#include "../audio_visual/overlay/animation.c"
#include "../audio_visual/overlay/render_overlay.c"

#include "level.c"
#include "input.c"
#include "gui.c"

#include "../combat/bfs_types.c"
#include "../combat/bfs.c"
#include "../combat/navigator.c"
#include "../combat/weapon.c"
#include "../combat/enemy.c"

#include "../audio_visual/floor_ceiling/floor_and_ceiling.c"
#include "../audio_visual/floor_ceiling/floor_and_ceiling_2.c"
#include "../audio_visual/floor_ceiling/floor_and_ceiling_3.c"
#include "../audio_visual/floor_ceiling/floor_and_ceiling_4.c"
#include "../audio_visual/floor_ceiling/draw_plane.c"
#include "../audio_visual/raycasting/render.c"
#include "../audio_visual/raycasting/render_2.c"

#include "../levels/level_1.c"
#include "../levels/debug_level.c"
#include "../levels/palace.c"
#include "../levels/red_room.c"

/*
itinerary:
a beautiful skybox
enemy ai
visplane floors
non-clipping enemies
*/

int main(void) {
	Player player;
	Weapon weapon;
	load_all_defaults(load_palace, &player, &weapon);
	// FloorCastThread floorcast_thread = init_floorcast_thread(&player);

	if (display_title_screen(&player.z_pitch, player.mouse_pos.y) == Exit)
		deinit_all(player, weapon);

	play_sound(current_level.background_sound, 1);

	while (1) {
		const Uint32 before = SDL_GetTicks();
		if (keys[SDL_SCANCODE_C]) DEBUG_VECF(player.pos);
		// player.pos = (VectorF) {3.0, 3.0};

		const InputStatus input_status = handle_input(&player, 0);
		if (input_status == Exit) deinit_all(player, weapon);

		update_screen_dimensions(&player.z_pitch, player.mouse_pos.y);
		prepare_for_drawing();

		const double wall_y_shift = settings.half_screen_height + player.z_pitch + player.pace.screen_offset;
		draw_skybox(player.angle, wall_y_shift);

		#ifndef PLANAR_MODE

		const double full_jump_height = player.jump.height * settings.screen_height;

		// start_floorcast_tick(&floorcast_thread);
		if (!keys[SDL_SCANCODE_T]) raycast_2(player, wall_y_shift, full_jump_height);
		draw_generic_billboards(player, wall_y_shift);

		draw_floor_plane(player);
		refresh_and_clear_temp_buf();

		update_all_enemies(player);
		use_weapon_if_needed(&weapon, player, input_status);
		// wait_for_floorcast_tick(&floorcast_thread);

		#else

		// draw_ceiling_plane(player);
		draw_floor_plane(player);
		refresh_and_clear_temp_buf();

		#endif

		refresh(player.tilt, player.pos);
		tick_delay(before);
	}
}
