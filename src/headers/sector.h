#ifndef SECTOR_H
#define SECTOR_H

#include "buffer_defs.h"
#include "list.h"
#include "camera.h"
#include "batch_draw_context.h"
#include "csm.h"

// Note: for batching sectors, extend Drawable by allowing an option to do a culling step modeled as dependency injection

typedef struct {
	const byte texture_id, origin[2];
	byte size[2]; // Top-down (X and Z); same for origin
	struct {byte min; const byte max;} visible_heights;
	struct {buffer_size_t start, length;} face_range; // Face domain that defines sector's faces; used for batching
} Sector;

typedef struct {
	const GLuint face_normal_map_set;
	BatchDrawContext draw_context;
	const List sectors;
} SectorContext;

/* Excluded: point_matches_sector_attributes, form_sector_area, generate_sectors_from_maps,
draw_sectors, make_aabb, get_renderable_index_from_cullable, get_num_renderable_from_cullable */

void draw_all_sectors_for_shadow_map(const void* const param);

void draw_visible_sectors(const SectorContext* const sector_context,
	const CascadedShadowContext* const shadow_context,
	const Camera* const camera, const GLint screen_size[2]);

SectorContext init_sector_context(const byte* const heightmap, const byte* const texture_id_map,
	const byte map_width, const byte map_height, const bool apply_normal_map_blur, const GLuint texture_set);

void deinit_sector_context(const SectorContext* const sector_context);

#endif
