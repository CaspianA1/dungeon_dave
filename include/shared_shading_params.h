#ifndef SHARED_SHADING_PARAMS_H
#define SHARED_SHADING_PARAMS_H

#include "uniform_buffer.h"
#include "list.h"
#include "shadow.h"
#include "camera.h"

// I would like to call `constant` here `static`, but that's a keyword
typedef struct {UniformBuffer constant, dynamic;} SharedShadingParams;

// Excluded: init_constant_shading_params

SharedShadingParams init_shared_shading_params(const GLuint* const shaders_that_share_params,
	const GLuint shader_count, const CascadedShadowContext* const shadow_context);

void deinit_shared_shading_parsms(const SharedShadingParams* const shared_shading_params);
void update_shared_shading_params(SharedShadingParams* const shared_shading_params,
	const Camera* const camera, const CascadedShadowContext* const shadow_context);

#endif
