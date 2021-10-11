#include "global_defs.h"

#include "../audio_visual/overlay/overlay.h"
#include "../audio_visual/sound.c"
#include "../combat/combat.h"

#include "../audio_visual/gui/gui.h"
#include "global_types.h"

#include "utils.c"
#include "dda.c"
#include "statemap.c"
#include "settings.c"
#include "bounding_box.c"
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

#include "../audio_visual/gui/gui_utils.c"
#include "../audio_visual/gui/menu.c"
#include "../audio_visual/gui/hud.c"

#include "../combat/bfs_types.c"
#include "../combat/bfs.c"
#include "../combat/navigator.c"
#include "../combat/weapon.c"
#include "../combat/enemy.c"
#include "../combat/health_kit.c"

#include "../data/menus.c"
#include "../data/enemies.c"

#include "../data/levels/level_1.c"
#include "../data/levels/debug_level.c"
#include "../data/levels/red_room.c"
#include "../data/levels/forever_maze.c"
#include "../data/levels/palace.c"
#include "../data/levels/mipmap_hallway.c"
#include "../data/levels/fleckenstein.c"
#include "../data/levels/pyramid.c"
#include "../data/levels/teleporter_turmoil.c"

/*
TODO:
audio todo:
	- find less crackly enemy sounds
	- call SDL_OpenAudio before Mix_LoadWAV
	- make enemy sound directions be constantly updated when they're playing

- sometimes, the mouse can escape the window when it shouldn't be able to
- bigger projectile size for inter-tick projectiles
- world collision detection with 3D bounding boxes
- in the window area of tpt, one of the enemies can attack the player through the wall
- don't deal damage if height diff too big for short range
- lightmap seed to init_level, or perlin shading in shader fn
- make sure that thread creation doesn't stall
- no more constant copying of many static amount things
- a vantage point map element
- stop player from hitting head by doing head-hit detection after jumping, and then stopping jumping after
- enemies won't chase you if you're far away enough
- a small occasional top stitch for floorcasting
- delta time for time-independent physics
- mipmapping for the floor (based on the wall height)
- bilinear filtering for the lightmap
- anisotropic filtering for angled walls
- the wall corner floor algorithm
- distance shading
- better antialiasing by sampling from just the last mip level
- a pause menu activated by esc, instead of escaping a window by pressing esc (screen size would be changed there too)
- long range AI
- a unique hitbox size for each thing, and can run through some things
- screen width and height to screen size via sublime text substitutions
*/

// drawing order: skybox, walls, things, weapon, floor, minimap, hp, crosshair
int main(void) {
	Player player;
	Weapon weapon;
	Player* const player_ref = &player;

	load_all_defaults(load_palace, player_ref, &weapon);
	if (display_title_screen() == Exit) deinit_all(player_ref, &weapon);

	play_sound(&current_level.background_sound);
	ground = init_pix_sprite("assets/walls/sand.bmp");

	#ifdef SHADING_ENABLED
	byte begin_level_fade = INIT_BEGIN_LEVEL_FADE;
	#endif

	byte running = 1;

	while (running) {
		const Uint32 before = SDL_GetTicks();
		if (keys[SDL_SCANCODE_C]) DEBUG_VEC(player.pos);

		update_screen_dimensions();
		const InputStatus input_status = handle_input(player_ref, player.is_dead);

		switch (input_status) {
			case Exit:
				running = 0;
				break;
			case ToOptionsMenu:
				display_options_menu();
				puts("Options menu");
				break;
			default: break;
		}

		const double horizon_line = settings.half_screen_height + player.y_pitch + player.pace.screen_offset;

		prepare_for_drawing();
		draw_skybox(player.angle, horizon_line);

		#ifndef PLANAR_MODE
		clear_statemap(occluded_by_walls);
		raycast(player_ref, horizon_line, player.jump.height);
		draw_things(player.pos, player.angle, player.jump.height, horizon_line);

		#ifndef DISABLE_ENEMIES
		if (!player.is_dead) update_all_enemy_instances(player_ref, &weapon);
		#endif

		use_weapon_if_needed(&weapon, player_ref, input_status);
		#else
		fill_val_buffers_for_planar_mode(player.angle);
		#endif

		if (player.is_dead && death_effect(player_ref))
			running = 0;

		parallel_floorcast(0, player.pos, player.jump.height, horizon_line);
		// floorcast(0, horizon_line, horizon_line, settings.screen_height, player.pos, player.jump.height);

		teleport_player_if_needed(player_ref);
		use_health_kit_if_needed(player_ref);

		#ifdef SHADING_ENABLED
		if (begin_level_fade != 255) {
			begin_level_fade++;
			SDL_SetTextureColorMod(screen.pixel_buffer, begin_level_fade, begin_level_fade, begin_level_fade);
			SDL_SetTextureColorMod(screen.shape_buffer, begin_level_fade, begin_level_fade, begin_level_fade);
		}
		#endif

		refresh(player_ref);
		tick_delay(before);
	}
	deinit_pix_sprite(ground);
	deinit_all(player_ref, &weapon);
}
