#ifndef SECTOR_CULLING_C
#define SECTOR_CULLING_C

#include "headers/sector_culling.h"
#include "headers/texture.h"

#include "sector.c"
#include "camera.c"

/* This code culls the sectors outside of the view frustum
And puts the indices of the visible ones into a batch
to be rendered in one draw call via glDrawElements. */

#endif
