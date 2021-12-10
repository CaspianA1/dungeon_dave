#ifndef CULLING_H
#define CULLING_H

#include "utils.h"
#include "sector.h"
#include "camera.h"

// Excluded: sector_in_view_frustum, draw_sectors_in_view_frustum

void draw_sectors(const DrawableSet* const sector_list, const Camera* const camera, const GLuint sector_shader);

#endif
