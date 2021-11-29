#ifndef CULLING_H
#define CULLING_H

#include "utils.h"
#include "sector.h"

byte sector_in_view_frustum(const Sector sector, vec4 frustum_planes[6]);
void draw_sectors_in_view_frustum(const SectorList* const sector_list, const Camera* const camera);

#endif
