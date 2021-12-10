#ifndef FACE_H
#define FACE_H

#include "sector.h"
#include "list.h"

/* NS - north-south, and EW = east-west.
If a face is NS, its two ends lie on a vertical top-down axis;
and if a face is EW, its two ends lie on a horizontal axis. */
typedef enum {
	Flat, Vert_NS, Vert_EW
} FaceType;

/* Faces don't store their height origin, since sectors store that.
For vert faces, origin and size[0] are top-down, and size[1] is depth.
For hori faces, origin and size are both top-down. */
typedef struct {
	const FaceType type;
	byte origin[2], size[2];
} Face;

// Excluded: print_face, get_next_face, init_vert_faces, add_face_mesh_to_list

void init_face_mesh_and_sector_lists(DrawableSet* const sector_list,
	List* const face_mesh_list, const byte* const heightmap,
	const byte* const texture_id_map, const byte map_width, const byte map_height);

void init_sector_list_vbo_and_ibo(DrawableSet* const sector_list, const List* const face_list);

#endif
