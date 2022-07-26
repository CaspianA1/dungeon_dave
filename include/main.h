#ifndef MAIN_H
#define MAIN_H

#include "buffer_defs.h"
#include "camera.h"
#include "shared_shading_params.h"
#include "weapon_sprite.h"
#include "sector.h"
#include "billboard.h"
#include "shadow.h"
#include "skybox.h"
#include "title_screen.h"

/* Drawing architecture change, plan:
1. Allow BatchDrawContext to call glDrawArraysInstanced, if needed
2. Abandon BatchDrawContext, and replace the current frustum culling code with some code that uses details from Drawable

3. Figure out how to get a texture with an alpha channel to render to the shadow map.
	Perhaps make some function in `shadow.c` that enables/disables alpha testing of a shadow
	texture? If not, that texture could simply be set to a 1x1 texture, with a single alpha value of 1 (so all alpha tests pass).
	That texture would be a texture array, where a frame is specified (current frame index of weapon would equal the frame there).
	Note: the alpha test should not change the color of the shadow; it should only change the strength of it.

4. Figure out how to render the weapon sprite to the shadow map
5. If needed, render a flat character or 3D character model with it, so that the weapon isn't floating

6. Add a dependency injection step for Drawable that allows a shadow map rendering step.
	Use that step for sectors now, and billboards and the weapon sprite later

7. Make a definitive plan on how to render billboards to the shadow map, and then do so
8. Draw sectors in an instanced manner: for each face, define a origin point, size, and face info bits (that encodes the texture id and face type)
9. Make a sector mesh buffer optimized for the shadow map: back faces pre-culled,
	face info bits removed, map edge faces added, and in a GL_STATIC_DRAW buffer

10.  Make an R-tree implementation for static objects, namely sectors here
11.  Make it so that the R-tree can be dynamic too, so for billboards
12. Replace `cull_from_frustum_into_gpu_buffer` with this recursive culling -> GPU buffer method
13. Allow frustum culling to act as a dependency injection step into Drawable, so that in the end,
	everything uses Drawable
*/

// TODO: add more const qualifiers where I can
typedef struct {
	Camera camera;

	SharedShadingParams shared_shading_params;

	WeaponSprite weapon_sprite;
	const SectorContext sector_context;
	BillboardContext billboard_context;

	CascadedShadowContext shadow_context;

	const Skybox skybox;
	TitleScreen title_screen;

	const byte* const heightmap, map_size[2];
} SceneContext;

// Excluded: draw_all_objects_to_shadow_map, main_drawer, main_init, main_deinit

#endif
