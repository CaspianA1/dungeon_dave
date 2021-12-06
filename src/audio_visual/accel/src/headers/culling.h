#ifndef CULLING_H
#define CULLING_H

#include "utils.h"
#include "sector.h"

// Excluded: sector_in_view_frustum

void draw_sectors_in_view_frustum(const SectorList* const sector_list, const Camera* const camera);

#endif
