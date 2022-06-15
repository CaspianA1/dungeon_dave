#ifndef MAIN_H
#define MAIN_H

#include "buffer_defs.h"
#include "weapon_sprite.h"
#include "batch_draw_context.h"
#include "billboard.h"
#include "csm.h"
#include "camera.h"
#include "list.h"
#include "skybox.h"
#include "title_screen.h"

// TODO: add more const qualifiers where I can
typedef struct {
	Camera camera;

	WeaponSprite weapon_sprite;

	//////////

	BatchDrawContext sector_draw_context;
	GLuint face_normal_map_set;
	List sectors;

	//////////

	const BillboardContext billboard_context;

	CascadedShadowContext cascaded_shadow_context;

	const Skybox skybox;
	TitleScreen title_screen;

	const byte *const heightmap, *const texture_id_map, map_size[2];
} SceneContext;

// Excluded: main_init, main_drawer, main_deinit

#endif
