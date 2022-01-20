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
	struct {buffer_size_t start, length;} face_range; // face domain that defines sector's faces; used for batching
} Sector;

// Excluded: point_matches_sector_attributes, form_sector_area, sector_in_view_frustum, draw_sectors

List generate_sectors_from_maps(
	const byte* const heightmap, const byte* const texture_id_map,
	const byte map_width, const byte map_height);

void init_sector_draw_context(
	BatchDrawContext* const draw_context, List* const face_meshes_ref, const byte* const heightmap,
	const byte* const texture_id_map, const byte map_width, const byte map_height);

void draw_visible_sectors(const BatchDrawContext* const draw_context, const List* const sector_face_meshes,
	const Camera* const camera, const GLuint perlin_texture);

#endif
