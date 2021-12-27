#ifndef SECTOR_H
#define SECTOR_H

#include "constants.h"
#include "buffer_defs.h"
#include "list.h"
#include "camera.h"
#include "batch_draw_context.h"

typedef struct {
	const byte texture_id, origin[2];
	byte size[2]; // Top-down (X and Z); same for origin
	struct {byte min; const byte max;} visible_heights;
	struct {buffer_index_t start, length;} ibo_range; // ibo domain that defines sector's faces
} Sector;

// Excluded: point_matches_sector_attributes, form_sector_area, sector_in_view_frustum, draw_sectors

byte* map_point(byte* const map, const byte x, const byte y, const byte map_width);

List generate_sectors_from_maps(
	const byte* const heightmap, const byte* const texture_id_map,
	const byte map_width, const byte map_height);

void init_sector_draw_context(
	IndexedBatchDrawContext* const draw_context, const byte* const heightmap,
	const byte* const texture_id_map, const byte map_width, const byte map_height);

void draw_visible_sectors(const IndexedBatchDrawContext* const indexed_draw_context, const Camera* const camera);

#endif
