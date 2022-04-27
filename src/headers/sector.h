#ifndef SECTOR_H
#define SECTOR_H

#include "buffer_defs.h"
#include "list.h"
#include "camera.h"
#include "batch_draw_context.h"
#include "evsm.h"
#include "texture.h"

typedef struct {
	const byte texture_id, origin[2];
	byte size[2]; // Top-down (X and Z); same for origin
	struct {byte min; const byte max;} visible_heights;
	struct {buffer_size_t start, length;} face_range; // Face domain that defines sector's faces; used for batching
} Sector;

/* Excluded: point_matches_sector_attributes, form_sector_area,
sector_in_view_frustum, draw_sectors, fill_sector_vbo_with_visible_faces */

List generate_sectors_from_maps(const byte* const heightmap,
	const byte* const texture_id_map, const byte map_width, const byte map_height);

void init_sector_draw_context(BatchDrawContext* const draw_context,
	List* const sectors_ref, const byte* const heightmap,
	const byte* const texture_id_map, const byte map_width, const byte map_height);

void draw_visible_sectors(const BatchDrawContext* const draw_context,
	const ShadowMapContext* const shadow_map_context, const List* const sector_face_meshes,
	const Camera* const camera, const GLuint normal_map_set, const int screen_size[2]);

#endif
