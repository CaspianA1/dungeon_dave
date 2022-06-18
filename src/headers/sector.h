#ifndef SECTOR_H
#define SECTOR_H

#include "buffer_defs.h"
#include "list.h"
#include "camera.h"
#include "batch_draw_context.h"
#include "csm.h"

typedef struct {
	const byte texture_id, origin[2];
	byte size[2]; // Top-down (X and Z); same for origin
	struct {byte min; const byte max;} visible_heights;
	struct {buffer_size_t start, length;} face_range; // Face domain that defines sector's faces; used for batching
} Sector;

/* Excluded: point_matches_sector_attributes, form_sector_area, draw_sectors,
make_aabb, get_renderable_index_from_cullable, get_num_renderable_from_cullable */

List generate_sectors_from_maps(const byte* const heightmap,
	const byte* const texture_id_map, const byte map_width, const byte map_height);

void init_sector_draw_context(BatchDrawContext* const draw_context, List* const sectors,
	const byte* const heightmap, const byte* const texture_id_map, const byte map_width, const byte map_height);

void draw_all_sectors_for_shadow_map(const void* const param);

void draw_visible_sectors(const BatchDrawContext* const draw_context,
	const CascadedShadowContext* const shadow_context, const List* const sectors,
	const Camera* const camera, const GLuint normal_map_set, const GLint screen_size[2]);

#endif
