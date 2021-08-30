#include "global_defs.h"
#include "../audio_visual/sound.c"
#include "../audio_visual/overlay/overlay.h"
#include "../combat/combat.h"
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
#include "../audio_visual/lighting.c"

#include "../audio_visual/blur_filter.c"
#include "../audio_visual/mipmap.c"

#include "../audio_visual/overlay/sprite.c"
#include "../audio_visual/overlay/animation.c"
#include "../audio_visual/overlay/things.c"

#include "../audio_visual/misc.c"
#include "../audio_visual/raycast.c"
#include "../audio_visual/floorcast.c"

#include "../audio_visual/gui/gui.h"
#include "../audio_visual/gui/gui.c"
#include "../audio_visual/gui/title_screen.c"
#include "../audio_visual/gui/hud.c"
#include "../audio_visual/gui/option_screen.c"

#include "../combat/bfs_types.c"
#include "../combat/bfs.c"
#include "../combat/navigator.c"
#include "../combat/weapon.c"
#include "../combat/enemy.c"

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
- proper mipmap blurring
- the polygon floor algorithm
- 3D DDA pitch-angle translation
- arms on the trooper
- different enemy AIs
- a unique hitbox size for each billboard
- correct height translation, and then stop rendering early if possible
- screen width and height to screen size via sublime text substitutions
- heightmaps
*/

// drawing order: skybox, walls, things, weapon, floor, minimap, hp, crosshair
int main(void) {
	Player player;
	Weapon weapon;
	load_all_defaults(load_palace, &player, &weapon);

	if (display_title_screen() == Exit) deinit_all(&player, &weapon);
	play_sound(&current_level.background_sound, 1);
	p = init_psprite("assets/walls/dune.bmp");

	while (1) {
		const Uint32 before = SDL_GetTicks();
		if (keys[SDL_SCANCODE_C]) DEBUG_VEC(player.pos);

		teleport_player_if_needed(&player);
		const InputStatus input_status = handle_input(&player, player.is_dead);
		if (input_status == Exit) deinit_all(&player, &weapon);

		update_screen_dimensions(&player.y_pitch, player.mouse_pos.y);

		const double wall_y_shift = settings.half_screen_height + player.y_pitch + player.pace.screen_offset;

		prepare_for_drawing();
		draw_skybox(player.angle, wall_y_shift);
		// draw_colored_floor(wall_y_shift);

		#ifndef PLANAR_MODE
		raycast(&player, wall_y_shift, player.jump.height * settings.screen_height); // screen b/c height is vertical

		draw_things(&player, wall_y_shift);

		#ifndef DISABLE_ENEMIES
		if (!player.is_dead) update_all_enemy_instances(&player, &weapon);
		#endif

		use_weapon_if_needed(&weapon, &player, input_status);
		#else
		fill_val_buffers_for_planar_mode(player.angle);
		#endif

		if (player.is_dead && death_effect(&player))
			deinit_all(&player, &weapon);

		fast_affine_floor(0, player.pos, player.jump.height, player.pace.screen_offset, wall_y_shift, player.y_pitch);

		refresh(&player, wall_y_shift);
		tick_delay(before);
	}
}
