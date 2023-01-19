#ifndef SECTOR_H
#define SECTOR_H

#include "utils/typedefs.h" // For OpenGL types + other typedefs
#include "rendering/drawable.h" // For `Drawable`
#include "utils/list.h" // For `List`
#include "camera.h" // For `Camera`
#include "level_config.h" // For `MaterialPropertiesPerObjectType`
#include "rendering/dynamic_light.h" // For `DynamicLightConfig`

// These definitions are in the header so that face.c can use them too
typedef face_component_t face_vertex_t[components_per_face_vertex];
typedef face_vertex_t face_mesh_t[vertices_per_face];

typedef struct {
	const byte origin[2];
	byte size[2]; // Top-down (X and Z); same for origin
	struct {byte min; const byte max;} visible_heights;
	struct {buffer_size_t start, length;} face_range; // Face domain that defines sector's faces; used for batching
} Sector;

typedef struct {
	const Drawable drawable;

	/* There's info on why there's a separate vertex buffer + spec in the
	comment above `init_trimmed_face_mesh_for_shadow_mapping`. */
	const struct {
		const GLsizei num_vertices;
		const GLuint vertex_buffer, vertex_spec, depth_shader;
	} shadow_mapping;

	const GLuint depth_prepass_shader;

	const List mesh_cpu, sectors;
} SectorContext;

/* Excluded:
point_matches_sector_attributes, form_sector_area,
generate_sectors_and_face_mesh_from_maps, init_trimmed_face_mesh_for_shadow_mapping,
frustum_cull_sector_faces_into_gpu_buffer, define_vertex_spec */

SectorContext init_sector_context(const byte* const heightmap,
	const byte* const texture_id_map, const byte map_width, const byte map_height,
	const GLchar* const* const texture_paths, const byte num_textures,
	const MaterialPropertiesPerObjectType* const shared_material_properties,
	const DynamicLightConfig* const dynamic_light_config);

void deinit_sector_context(const SectorContext* const sector_context);

void draw_sectors_to_shadow_context(const SectorContext* const sector_context);
void draw_sectors(const SectorContext* const sector_context, const Camera* const camera);

#endif
