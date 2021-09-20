#include "global_defs.h"
#include "../audio_visual/sound.c"

#include "../audio_visual/overlay/overlay.h"
#include "../combat/combat.h"

#include "../audio_visual/gui/gui.h"
#include "global_types.h"

#include "utils.c"
#include "dda.c"
#include "statemap.c"
#include "settings.c"
#include "movement.c" // previously a part of input.c
#include "input.c"
#include "teleport.c"
#include "level.c"

#include "../audio_visual/screen.c"
#include "../audio_visual/perlin.c"
#include "../audio_visual/lighting.c"
#include "../audio_visual/mipmap.c"

#include "../audio_visual/overlay/sprite.c"
#include "../audio_visual/overlay/animation.c"
#include "../audio_visual/overlay/things.c"

#include "../audio_visual/misc.c"
#include "../audio_visual/raycast.c"
#include "../audio_visual/floorcast.c"
#include "../audio_visual/parallel_floorcast.c"

#include "../audio_visual/gui/gui_utils.c"
#include "../audio_visual/gui/menu.c"
#include "../audio_visual/gui/hud.c"

#include "../combat/bfs_types.c"
#include "../combat/bfs.c"
#include "../combat/navigator.c"
#include "../combat/weapon.c"
#include "../combat/enemy.c"

#include "../audio_visual/gui/menu_data.c"
#include "../data/enemies.c"

#include "../data/levels/level_1.c"
#include "../data/levels/debug_level.c"
#include "../data/levels/red_room.c"
#include "../data/levels/forever_maze.c"
#include "../data/levels/palace.c"
#include "../data/levels/mipmap_hallway.c"
#include "../data/levels/fleckenstein.c"

/*
TODO:
- close-range attacks still don't work
- delta time for time-independent physics
- increase shotgun loudness and eddie death loudness, and make the zap noise quieter
- nth_bit_to_x should use bitmasks in some way
- a blurred skybox
- bilinear filtering for the lightmap
- the wall corner floor algorithm
- threaded floorcast doesn't work yet
- a small point_height function
- sometimes, a delay when pressing start
- a stitch for floorcasting
- cannot wrap the mouse from the left to the right for a full-size screen
- purple vertical scanlines for a full-size menu
- an odd thin line on the bottom of the screen for the colored floor, and the top of each wall
- distance shading
- better antialiasing by sampling from just the last mip level
- a pause menu activated by esc, instead of escaping a window by pressing esc (screen size would be changed there too)
- the rest of the trooper animations + long range AI
- a unique hitbox size for each thing, and can run through some things
- screen width and height to screen size via sublime text substitutions
- heightmaps
*/

// drawing order: skybox, walls, things, weapon, floor, minimap, hp, crosshair
int main(void) {
	Player player;
	Weapon weapon;
	load_all_defaults(load_hallway, &player, &weapon);

	if (display_title_screen() == Exit) deinit_all(&player, &weapon);

	play_sound(&current_level.background_sound, 1);
	ground = init_pix_sprite("assets/walls/mesa.bmp");

	while (1) {
		const Uint32 before = SDL_GetTicks();
		if (keys[SDL_SCANCODE_C]) DEBUG_VEC(player.pos);

		update_screen_dimensions();
		const InputStatus input_status = handle_input(&player, player.is_dead);

		switch (input_status) {
			case Exit:
				deinit_pix_sprite(ground);
				deinit_all(&player, &weapon);
				break;
			case OptionsMenu:
				puts("Options menu");
				break;
			default: break;

		}

		const double horizon_line = settings.half_screen_height + player.y_pitch + player.pace.screen_offset;

		prepare_for_drawing();
		draw_skybox(player.angle, horizon_line);

		#ifndef PLANAR_MODE
		raycast(&player, horizon_line, player.jump.height);
		draw_things(player.pos, to_radians(player.angle), player.jump.height, horizon_line);

		#ifndef DISABLE_ENEMIES
		if (!player.is_dead) update_all_enemy_instances(&player, &weapon);
		#endif

		use_weapon_if_needed(&weapon, &player, input_status);
		#else
		fill_val_buffers_for_planar_mode(player.angle);
		#endif

		if (player.is_dead && death_effect(&player))
			deinit_all(&player, &weapon);

		// parallel_floorcast(3, 0, player.pos, player.jump.height, horizon_line);
		fast_affine_floor(0, horizon_line, horizon_line, settings.screen_height, player.pos, player.jump.height);
		// fast_affine_floor(0, horizon_line, settings.half_screen_height, settings.screen_height, player.pos, player.jump.height);

		teleport_player_if_needed(&player);

		refresh(&player);
		tick_delay(before);
	}
}
