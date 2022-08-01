#ifndef AMBIENT_OCCLUSION_H
#define AMBIENT_OCCLUSION_H

#include "utils/buffer_defs.h"

typedef GLuint AmbientOcclusionMap;

AmbientOcclusionMap init_ao_map(void);
void deinit_ao_map(const AmbientOcclusionMap ao_map);

#endif
