#ifndef SHARED_SHADING_PARAMS_H
#define SHARED_SHADING_PARAMS_H

#include "utils/uniform_buffer.h"
#include "rendering/shadow.h"
#include "camera.h"

// These #includes pertain to `init_shared_textures_for_world_shaded_objects`
#include "rendering/drawable.h"
#include "utils/texture.h"
#include "utils/buffer_defs.h"
#include "rendering/entities/skybox.h"
#include "rendering/ambient_occlusion.h"

// I would like to call `constant` here `static`, but that's a keyword
typedef struct {UniformBuffer constant, dynamic;} SharedShadingParams;

// Excluded: init_constant_shading_params

SharedShadingParams init_shared_shading_params(const GLuint* const shaders_that_share_params,
	const GLuint num_shaders, const CascadedShadowContext* const shadow_context);

void deinit_shared_shading_params(const SharedShadingParams* const shared_shading_params);

void update_shared_shading_params(SharedShadingParams* const shared_shading_params,
	const Camera* const camera, const CascadedShadowContext* const shadow_context,
	const vec3 dir_to_light);

//////////

/* World-shaded objects use the `world_shading.vert` and
`.frag` shaders, and appear in the 3D game world. */
typedef struct {
	const Drawable* const drawable;
	const struct {const TextureUnit diffuse, normal_map;} texture_units;
} WorldShadedObject;

void init_shared_textures_for_world_shaded_objects(
	const WorldShadedObject* const world_shaded_objects,
	const byte num_world_shaded_objects, const Skybox* const skybox,
	const CascadedShadowContext* const shadow_context,
	const AmbientOcclusionMap* const ao_map);

#endif
