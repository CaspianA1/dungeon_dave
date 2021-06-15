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

/*
itinerary:
enemy ai
visplane floors
non-clipping enemies
*/

// this is the first commit from the new computer!

int main(void) {
	Player player;
	Weapon weapon;
	load_all_defaults(load_palace, &player, &weapon);
	// FloorCastThread floorcast_thread = init_floorcast_thread(&player);

	if (display_title_screen(&player.pace.domain.max) == Exit)
		deinit_all(player, weapon);

	play_sound(current_level.background_sound, 1);

	while (1) {
		const Uint32 before = SDL_GetTicks();
		if (keys[SDL_SCANCODE_C]) DEBUG_VECF(player.pos);

		const InputStatus input_status = handle_input(&player, 0);
		if (input_status == Exit) deinit_all(player, weapon);

		update_screen_dimensions(&player.pace.domain.max);
		prepare_for_drawing();

		// start_floorcast_tick(&floorcast_thread);
		raycast_2(player);
		draw_generic_billboards(player);

		/*
		draw_ceiling_plane(player);
		draw_floor_plane(player);
		refresh_and_clear_temp_buf();
		*/

		update_all_enemies(player);
		use_weapon_if_needed(&weapon, player, input_status);
		// wait_for_floorcast_tick(&floorcast_thread);

		refresh(player.tilt, player.pos, player.angle);
		tick_delay(before);
	}
}
