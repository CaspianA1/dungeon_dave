#ifndef SKYBOX_H
#define SKYBOX_H

#include "rendering/drawable.h" // For `Drawable`
#include "cglm/cglm.h" // For various cglm defs

typedef struct {
	const GLchar* const texture_path;
	const GLfloat texture_scale, horizon_dist_scale;
	const vec3 rotation_degrees_per_axis;

	// No cylindrical projection is applied if both widths are 0
	const vec2 cylindrical_cap_blend_widths;
} SkyboxConfig;

typedef struct {
	const Drawable drawable;
} Skybox;

//////////

// Excluded: init_skybox_texture, define_vertex_spec

Skybox init_skybox(const SkyboxConfig* const config);
void deinit_skybox(const Skybox* const skybox);
void draw_skybox(const Skybox* const skybox);

#endif
