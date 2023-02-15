#ifndef SKYBOX_H
#define SKYBOX_H

#include "rendering/drawable.h" // For `Drawable`

// TODO: how to keep the const qualifiers?
typedef struct {
	byte level_size[3]; // Width, max y, and height
	vec3 scale_ratios;
	GLfloat output_texture_scale, percentage_towards_y_bottom;
} SkyboxSphericalDistortionConfig;

typedef struct {
    const GLchar* const texture_path;
    const SkyboxSphericalDistortionConfig* const spherical_distortion_config;
} SkyboxConfig;

//////////

typedef struct {
    const Drawable drawable;
} Skybox;

/* Excluded: init_skybox_texture, define_skybox_vertex_spec, define_sphere_mesh_vertex_spec,
make_matrices_for_skybox_predistortion, make_spherically_distorted_skybox_texture */

Skybox init_skybox(const SkyboxConfig* const config);
void deinit_skybox(const Skybox* const skybox);
void draw_skybox(const Skybox* const skybox);

#endif
