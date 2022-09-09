#ifndef FACE_H
#define FACE_H

#include "utils/buffer_defs.h"
#include "rendering/entities/sector.h"
#include "utils/list.h"

////////// Excluded: print_face, get_next_face, add_to_face_meshes

void init_mesh_for_sector(
	const Sector* const sector, List* const face_meshes,
	byte* const biggest_face_height, const byte* const heightmap,
	const byte map_width, const byte map_height, const byte texture_id);

#endif
