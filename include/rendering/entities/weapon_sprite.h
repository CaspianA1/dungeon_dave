#ifndef WEAPON_SPRITE_H
#define WEAPON_SPRITE_H

#include "glad/glad.h" // For OpenGL defs
#include "utils/typedefs.h" // For various typedefs
#include "animation.h" // For `Animation`, and `AnimationLayout`
#include "cglm/cglm.h" // For `vec3`
#include "data/constants.h" // For `corners_per_quad`
#include "level_config.h" // For `MaterialPropertiesPerObjectType`
#include "rendering/drawable.h" // For `Drawable`
#include "utils/normal_map_generation.h" // For `NormalMapCreator`
#include "camera.h" // For `Camera`
#include "event.h" // For `Event`
#include "openal/al.h" // For various OpenAL defs

typedef struct {
	GLfloat cycle_start_time;
	texture_id_t texture_id;
	bool activated_in_this_tick, is_currently_being_animated;
	const Animation animation;
} WeaponSpriteAnimationContext;

typedef struct {
	const struct {
		const GLfloat
			frame_width_over_height, size,
			max_movement_magnitude,
			half_movement_cycles_per_sec;
	} screen_space;

	struct {
		const GLfloat max_yaw, max_pitch;
		vec3 corners[corners_per_quad];
	} world_space;
} WeaponSpriteAppearanceContext;

//////////

typedef struct {
	const struct {const GLfloat yaw, pitch;} max_degrees;
	const GLfloat secs_per_movement_cycle;

	const GLfloat screen_space_size, max_movement_magnitude;
	const AnimationLayout animation_layout;
	const MaterialPropertiesPerObjectType shared_material_properties;
	const ALchar* const sound_path;
} WeaponSpriteConfig;

typedef struct {
	const Drawable drawable;
	const GLuint depth_prepass_shader;

	const struct {const GLint frame_index, world_corners, tbn;} world_space_uniform_ids;
	const struct {const GLint frame_index, world_corners;} depth_prepass_uniform_ids;

	WeaponSpriteAnimationContext animation_context;
	WeaponSpriteAppearanceContext appearance_context;

	// These are used for OpenAL
	vec3 curr_sound_emitting_pos, velocity;
	bool openal_variables_are_uninitialized;
} WeaponSprite;

//////////

/* Excluded: update_weapon_sprite_animation, circular_mapping_from_zero_to_one,
get_screen_corners, get_world_corners, get_quad_tbn_matrix, update_uniforms,
define_vertex_spec, get_sound_emitting_pos */

////////// Drawing functions

WeaponSprite init_weapon_sprite(
	const WeaponSpriteConfig* const config,
	const NormalMapCreator* const normal_map_creator,
	const material_index_t material_index);

void deinit_weapon_sprite(const WeaponSprite* const ws);

void update_weapon_sprite(WeaponSprite* const ws, const Camera* const camera, const Event* const event);
void draw_weapon_sprite_to_shadow_context(const WeaponSprite* const ws); // TODO: make this work, and use it
void draw_weapon_sprite_for_depth_prepass(const WeaponSprite* const ws);
void draw_weapon_sprite(const WeaponSprite* const ws);

////////// Sound functions

bool weapon_sound_activator(const void* const data);
void weapon_sound_updater(const void* const data, const ALuint al_source);

#endif
