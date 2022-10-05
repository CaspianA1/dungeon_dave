#ifndef SECTOR_H
#define SECTOR_H

#include "utils/buffer_defs.h"
#include "rendering/drawable.h"
#include "utils/list.h"
#include "camera.h"
#include "normal_map_generation.h"

typedef struct {
	const byte origin[2];
	byte size[2]; // Top-down (X and Z); same for origin
	struct {byte min; const byte max;} visible_heights;
	struct {buffer_size_t start, length;} face_range; // Face domain that defines sector's faces; used for batching
} Sector;

typedef struct {
	const Drawable drawable;
	const GLuint depth_shader;
	const List mesh_cpu, sectors;
} SectorContext;

/* Excluded: point_matches_sector_attributes, form_sector_area, generate_sectors_from_maps,
frustum_cull_sector_faces_into_gpu_buffer, define_vertex_spec */

SectorContext init_sector_context(const byte* const heightmap,
	const byte* const texture_id_map, const byte map_width, const byte map_height,
	const GLchar* const* const texture_paths, const GLsizei num_textures,
	const GLsizei texture_size, const NormalMapConfig* const normal_map_config);

void deinit_sector_context(const SectorContext* const sector_context);

void draw_sectors_to_shadow_context(const SectorContext* const sector_context);
void draw_sectors(const SectorContext* const sector_context, const vec4 frustum_planes[planes_per_frustum]);

#endif
