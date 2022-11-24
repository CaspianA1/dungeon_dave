#ifndef FACE_H
#define FACE_H

#include "rendering/entities/sector.h" // For `Sector`, and the face component + vertex typedefs
#include "utils/list.h" // For `List`
#include "utils/typedefs.h" // For `byte`

////////// Excluded: print_face, get_next_face, add_to_face_mesh, init_map_edge_mesh

void init_mesh_for_sector(
	const Sector* const sector, List* const face_mesh,
	byte* const biggest_face_height, const byte* const heightmap,
	const byte map_width, const byte map_height, const byte texture_id);

/* Normally, the map edges are left out of the big sector sector, since they're not visible to the player,
but in the case of shadow-casting sectors, the map edges should cast shadows. So for that, this generates
another mesh for just the map edges that will provide correct map-edge shadows for the scene. */
List init_map_edge_mesh(const byte* const heightmap, const byte map_width, const byte map_height);

#endif
