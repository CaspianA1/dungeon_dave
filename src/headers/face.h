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

// Excluded: print_face, get_next_face. init_vert_faces and add_face_mesh_to_list are only used by sector.c

void init_vert_faces(
	const Sector sector, List* const face_mesh_list,
	const byte* const heightmap, const byte map_width,
	const byte map_height, byte* const biggest_face_height);

void add_face_mesh_to_list(const Face face, const byte sector_max_visible_height,
	const byte side, const byte texture_id, const byte map_width,
	const byte* const heightmap, List* const face_mesh_list);

#endif
