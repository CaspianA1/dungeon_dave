#ifndef MAIN_H
#define MAIN_H

#include "buffer_defs.h"
#include "camera.h"
#include "weapon_sprite.h"
#include "sector.h"
#include "billboard.h"
#include "shadow.h"
#include "skybox.h"
#include "title_screen.h"

/* Drawing architecture change, plan:
1. Figure out how to get a texture with an alpha channel to render to the shadow map

3. Figure out how to render the weapon sprite to the shadow map
4. If needed, render a flat character or 3D character model with it, so that the weapon isn't floating

5. Add a dependency injection step for Drawable that allows a shadow map rendering step.
	Use that step for sectors now, and billboards and the weapon sprite later

6. Make a definitive plan on how to render billboards to the shadow map, and then do so
7. Make a sector mesh buffer optimized for the shadow map: back faces pre-culled,
	face info bits removed, map edge faces added, and in a GL_STATIC_DRAW buffer

8.  Make an R-tree implementation for static objects, namely sectors here
9.  Make it so that the R-tree can be dynamic too, so for billboards
10. Replace `cull_from_frustum_into_gpu_buffer` with this recursive culling -> GPU buffer method
11. Remove BatchDrawContext, and replace it with this R-tree code as a new type of context
12. Allow this new type of context to act as a dependency injection step into Drawable,
	so that in the end, everything uses Drawable
*/

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
