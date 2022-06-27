#ifndef MAIN_H
#define MAIN_H

#include "buffer_defs.h"
#include "weapon_sprite.h"
#include "sector.h"
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
	const SectorContext sector_context;
	const BillboardContext billboard_context;

	CascadedShadowContext cascaded_shadow_context;

	const Skybox skybox;
	TitleScreen title_screen;

	const byte* const heightmap, map_size[2];
} SceneContext;

// Excluded: main_init, main_drawer, main_deinit

#endif
