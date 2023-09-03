#ifndef MAIN_H
#define MAIN_H

#include "audio.h" // For `AudioContext`
#include "utils/typedefs.h" // For various typedefs
#include "camera.h" // For `Camera`
#include "shared_shading_params.h" // For `SharedShadingParams`
#include "level_config.h" // For `MaterialsTexture`
#include "rendering/entities/weapon_sprite.h" // For `WeaponSprite`
#include "rendering/entities/sector.h" // For `SectorContext`
#include "rendering/entities/billboard.h" // For `BillboardContext`
#include "rendering/dynamic_light.h" // For `DynamicLight`
#include "rendering/shadow.h" // For `CascadedShadowContext`
#include "rendering/ambient_occlusion.h" // For `AmbientOcclusionMap`
#include "rendering/entities/skybox.h" // For `Skybox`
#include "rendering/entities/title_screen.h" // For `TitleScreen`

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
	AudioContext audio_context;

	Camera camera;

	SharedShadingParams shared_shading_params;
	const MaterialsTexture materials_texture;

	WeaponSprite weapon_sprite;
	const SectorContext sector_context;
	BillboardContext billboard_context;

	DynamicLight dynamic_light;
	CascadedShadowContext shadow_context;
	const AmbientOcclusionMap ao_map;

	const Skybox skybox;
	TitleScreen title_screen;

	const Heightmap heightmap;
} LevelContext;

// Excluded: main_init, main_init_with_path, main_deinit, main_drawer, cjson_wrapping_alloc

#endif
