#ifndef SECTOR_H
#define SECTOR_H

#include "buffer_defs.h"
#include "list.h"
#include "camera.h"
#include "shadow.h"
#include "skybox.h"
#include "normal_map_generation.h"

// Note: for batching sectors, extend Drawable by allowing an option to do a culling step modeled as dependency injection

typedef struct {
	const byte texture_id, origin[2];
	byte size[2]; // Top-down (X and Z); same for origin
	struct {byte min; const byte max;} visible_heights;
	struct {buffer_size_t start, length;} face_range; // Face domain that defines sector's faces; used for batching
} Sector;

typedef struct {
	const GLuint
		mesh_vertex_buffer, mesh_vertex_spec,
		diffuse_texture_set, normal_map_set, shader;

	const List mesh_cpu, sectors;
} SectorContext;

/* Excluded: point_matches_sector_attributes, form_sector_area, generate_sectors_from_maps,
internal_draw_sectors, frustum_cull_sector_faces_into_gpu_buffer */

void draw_all_sectors_to_shadow_context(const SectorContext* const sector_context);

void draw_sectors(const SectorContext* const sector_context,
	const CascadedShadowContext* const shadow_context,
	const Skybox* const skybox, const Camera* const camera,
	const GLfloat curr_time_secs);

SectorContext init_sector_context(const byte* const heightmap,
	const byte* const texture_id_map, const byte map_width, const byte map_height,
	const GLuint diffuse_texture_set, const NormalMapConfig* const normal_map_config);

void deinit_sector_context(const SectorContext* const sector_context);

#endif
