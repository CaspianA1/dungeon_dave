#ifndef BILLBOARD_H
#define BILLBOARD_H

#include "buffer_defs.h"
#include "batch_draw_context.h"
#include "camera.h"

typedef struct { // This struct is perfectly aligned
	bb_texture_id_t texture_id;
	bb_pos_component_t size[2], pos[3];
} Billboard;

// Excluded: is_inside_plane, billboard_in_view_frustum, draw_billboards

void draw_visible_billboards(const BatchDrawContext* const draw_context, const Camera* const camera);
BatchDrawContext init_billboard_draw_context(const size_t num_billboards, ...);

#endif
