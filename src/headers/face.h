#ifndef FACE_H
#define FACE_H

#include "buffer_defs.h"
#include "sector.h"
#include "list.h"

/* NS = north-south, and EW = east-west.
If a face is NS, its two ends lie on a vertical top-down axis;
and if a face is EW, its two ends lie on a horizontal top-down axis. */
typedef enum {Flat, Vert_NS, Vert_EW} FaceType;

/* Faces don't store their height origin, since sectors store that.
For vert faces, origin and size[0] are top-down, and size[1] is depth.
For hori faces, origin and size are both top-down. */
typedef struct {
	const FaceType type;
	byte origin[2], size[2];
} Face;

typedef face_mesh_component_t face_vertex_t[components_per_face_vertex];
typedef face_vertex_t face_mesh_t[vertices_per_face];

////////// Excluded: print_face, get_next_face, add_face_mesh_to_list, init_vert_faces.

List init_face_meshes_from_sectors(const List* const sectors,
	const byte* const heightmap, const byte map_width, const byte map_height);

#endif
