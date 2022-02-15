#include "../utils.c"
#include "../skybox.c"
#include "../data/maps.c"

#include "../sector.c"
#include "../billboard.c"
#include "../camera.c"
#include "../event.c"

#include "../overlay.c"

/*
- NEXT: final touches on new_map + a texmap for it
- NEXT 2: a sector BVH, through metasector trees, also called binary r-trees (alloc through node pool)
- NEXT 3: entities that don't turn to face the player (just static ones); defined by center, size, and normal
- NEXT 4: up-and-down moving platforms that can also work as doors (continually up-and-down moving, down if player close, or down if action fulfilled)
- NEXT 6: deprecate most of StateGL's members and rely solely on vertex_array and any_data

- Perlin noise-based lighting in 3D (maybe)
- Shadows + one global light source
- A map maker. An init json file that specifies textures and dimensions;
	draw/erase modes, line mode, export, and choose heights and textures
- More efficiently set statemap bit ranges, maybe
- Camera var names to yaw, pitch, and roll (maybe)
- Billboard lighting that matches the sector lighting (share uniforms via a uniform buffer object)
- Base darkest distance of attenuated light on the world size
- Can't use red cross for health since it's copyrighted
- Premultiplied billboard alphas: http://www.realtimerendering.com/blog/gpus-prefer-premultiplication/ (or perhaps not, since alpha just 0 or 1)

- Crouch
- Make deceleration and tilt decrease framerate-independent
- Animations go slower at 5 FPS
- Having an idle window with vsync on leads to high CPU and GPU usage
- Make tilting not depend on framerate (mouse deltas will always be smaller with a higher FPS)
- The checker heightmap has faster pacing sometimes
- Pressing left + forward and backward doesn't stop moving on the X axis
- A half-stutter when pressing towards a wall and then letting go of a movement key
- Can get a tiny bit stuck in a wall when jumping downwards close to a wall (so will have to be able to stand a tiny bit off the wall)

- Memory leak with debug mode at the end of demos 17 and 22 (rest probably too), the lightmapper, the map editor, and the collision demo
- Other demos untested
- Outer project does not even want to start
- Demo test range doesn't work
- Sometimes, mouse control does not work when using the keyboard
- Framerate drop when using the big monitor + optimized + full screen

- Blit 2D sprite to whole screen
- Flat weapon
- Use more of the cglm functions in `update_camera`, or make my own
- A startup setting of magnification filter (and setting other constants like that as well; call that config_constants)
- Avoid glew altogether and just use SDL_GL_GetProcAddress?

- In the end, 5 shaders + accel components: sectors, billboards, skybox, weapon, ui elements
*/

typedef struct {
	const GLuint lightmap_texture; // This is grayscale

	WeaponSprite weapon_sprite;

	BatchDrawContext sector_draw_context, billboard_draw_context;

	List sectors; // This is not in the sector draw context b/c the cpu list for that context consists of vertices
	const List billboard_animations, billboard_animation_instances;

	const Skybox skybox;
	byte* const heightmap;
	const byte map_size[2];
} SceneState;

StateGL demo_17_init(void) {
	StateGL sgl = {.vertex_array = init_vao(), .num_vertex_buffers = 0, .num_textures = 0};

	SceneState scene_state = {
		// "../assets/palace_perlin.bmp", "../assets/water_grayscale.bmp"
		.lightmap_texture = init_plain_texture("../assets/palace_perlin.bmp", TexPlain, TexNonRepeating, OPENGL_GRAYSCALE_INTERNAL_PIXEL_FORMAT),

		// .weapon_sprite = init_weapon_sprite(0.5f, 0.07f, "../../../../assets/spritesheets/weapons/desecrator_cropped.bmp", 1, 8, 8),
		.weapon_sprite = init_weapon_sprite(0.65f, 0.016f, "../../../../assets/spritesheets/weapons/whip.bmp", 4, 6, 22),

		.billboard_animations = LIST_INITIALIZER(animation) (4,
			(Animation) {.texture_id_range = {2, 47}, .secs_per_frame = 0.02f}, // Flying carpet
			(Animation) {.texture_id_range = {48, 52}, .secs_per_frame = 0.15f}, // Torch
			(Animation) {.texture_id_range = {61, 63}, .secs_per_frame = 0.08f}, // Eddie, attacking
			(Animation) {.texture_id_range = {76, 79}, .secs_per_frame = 0.07f} // Trooper, idle
		),

		.billboard_animation_instances = LIST_INITIALIZER(billboard_animation_instance) (6,
			(BillboardAnimationInstance) {.ids = {.billboard = 4, .animation = 0}, .last_frame_time = 0.0f}, // Flying carpet
			(BillboardAnimationInstance) {.ids = {.billboard = 5, .animation = 1}, .last_frame_time = 0.0f}, // Torch

			(BillboardAnimationInstance) {.ids = {.billboard = 6, .animation = 2}, .last_frame_time = 0.0f}, // Eddies
			(BillboardAnimationInstance) {.ids = {.billboard = 7, .animation = 2}, .last_frame_time = 0.0f},

			(BillboardAnimationInstance) {.ids = {.billboard = 8, .animation = 3}, .last_frame_time = 0.0f}, // Troopers
			(BillboardAnimationInstance) {.ids = {.billboard = 9, .animation = 3}, .last_frame_time = 0.0f}
		),

		.skybox = init_skybox("../assets/night.bmp"),
		.heightmap = (byte*) palace_heightmap,
		.map_size = {palace_width, palace_height}
	};

	//////////
	// static byte texture_id_map[terrain_height][terrain_width];
	init_sector_draw_context(&scene_state.sector_draw_context, &scene_state.sectors,
		(byte*) scene_state.heightmap, (byte*) palace_texture_id_map, scene_state.map_size);

	scene_state.billboard_draw_context = init_billboard_draw_context(
		10,
		(Billboard) {0, {1.0f, 1.0f}, {28.0f, 2.5f, 31.0f}}, // Health kits
		(Billboard) {0, {1.0f, 1.0f}, {5.0f, 0.5f, 22.5f}},

		(Billboard) {1, {1.0f, 1.0f}, {12.5f, 0.5f, 38.5f}}, // Teleporters
		(Billboard) {1, {1.0f, 1.0f}, {8.5f, 0.5f, 25.5f}},

		(Billboard) {2, {1.0f, 1.0f}, {5.0f, 0.5f, 2.0f}}, // Flying carpet
		(Billboard) {48, {1.0f, 1.0f}, {7.5f, 0.5f, 12.5f}}, // Torch

		(Billboard) {61, {1.0f, 1.0f}, {6.5f, 0.5f, 21.5f}}, // Eddies
		(Billboard) {61, {1.0f, 1.0f}, {3.5f, 0.5f, 24.5f}},

		(Billboard) {76, {1.0f, 1.0f}, {3.0f, 1.5f, 9.5f}}, // Troopers
		(Billboard) {76, {1.0f, 1.0f}, {21.5f, 0.5f, 24.5f}}
	);

	scene_state.billboard_draw_context.texture_set = init_texture_set(
		TexNonRepeating, 2, 4, 128, 128,

		"../../../../assets/objects/health_kit.bmp",
		"../../../../assets/objects/teleporter.bmp",

		"../../../../assets/spritesheets/flying_carpet.bmp", 5, 10, 46,
		"../../../../assets/spritesheets/torch_2.bmp", 2, 3, 5,
		"../../../../assets/spritesheets/eddie.bmp", 23, 1, 23,
		"../../../../assets/spritesheets/trooper.bmp", 33, 1, 33
	);

	//////////

	scene_state.sector_draw_context.texture_set = init_texture_set(TexRepeating,
		// Fortress:
		/* 4, 0, 256, 256,
		"../../../../assets/walls/viney_bricks.bmp",
		"../../../../assets/walls/marble.bmp",
		"../../../../assets/walls/vines.bmp",
		"../../../../assets/walls/stone_2.bmp" */

		// Palace:
		11, 0, 128, 128,
		"../../../../assets/walls/sand.bmp", "../../../../assets/walls/pyramid_bricks_4.bmp",
		"../../../../assets/walls/marble.bmp", "../../../../assets/walls/hieroglyph.bmp",
		"../../../../assets/walls/window.bmp", "../../../../assets/walls/saqqara.bmp",
		"../../../../assets/walls/sandstone.bmp", "../../../../assets/walls/cobblestone_3.bmp",
		"../../../../assets/walls/horses.bmp", "../../../../assets/walls/mesa.bmp",
		"../../../../assets/walls/arthouse_bricks.bmp"

		// Pyramid:
		/* 3, 0, 512, 512, "../../../../assets/walls/pyramid_bricks_4.bmp",
		"../../../../assets/walls/greece.bmp", "../../../../assets/walls/saqqara.bmp" */

		// Tiny:
		// 2, 0, 64, 64, "../../../../assets/walls/mesa.bmp", "../../../../assets/walls/hieroglyph.bmp"

		// Level 1:
		/* 8, 0, 256, 256, "../../../../assets/walls/sand.bmp",
		"../../../../assets/walls/cobblestone_2.bmp",
		"../../../../assets/walls/cobblestone_3.bmp",
		"../../../../assets/walls/stone_2.bmp",
		"../../../../assets/walls/pyramid_bricks_3.bmp",
		"../../../../assets/walls/hieroglyphics.bmp",
		"../../../../assets/walls/desert_snake.bmp",
		"../../../../assets/wolf/colorstone.bmp" */
		);

	enable_all_culling();
	glEnable(GL_MULTISAMPLE);

	sgl.any_data = malloc(sizeof(SceneState));
	memcpy(sgl.any_data, &scene_state, sizeof(SceneState));

	return sgl;
}

void demo_17_drawer(const StateGL* const sgl) {
	SceneState* const scene_state = (SceneState*) sgl -> any_data;

	static Camera camera;
	static PhysicsObject physics_obj;
	static byte first_call = 1;

	if (first_call) {
		init_camera(&camera, (vec3) {1.5f, 0.5f, 1.5f}); // {3.9f, 0.5f, 6.0f}, {12.5f, 3.5f, 22.5f}
		physics_obj.heightmap = scene_state -> heightmap;
		physics_obj.map_size[0] = scene_state -> map_size[0];
		physics_obj.map_size[1] = scene_state -> map_size[1];
		first_call = 0;
	}

	const Event event = get_next_event();

	update_billboard_animation_instances(
		&scene_state -> billboard_animation_instances,
		&scene_state -> billboard_animations,
		&scene_state -> billboard_draw_context.buffers.cpu);

	update_camera(&camera, event, &physics_obj);

	// Skybox after sectors b/c most skybox fragments would be unnecessarily drawn otherwise
	draw_visible_sectors(&scene_state -> sector_draw_context, &scene_state -> sectors,
		&camera, scene_state -> lightmap_texture, scene_state -> map_size);

	draw_skybox(scene_state -> skybox, &camera);
	draw_visible_billboards(&scene_state -> billboard_draw_context, &camera);
	update_and_draw_weapon_sprite(&scene_state -> weapon_sprite, &camera, &event);
}

void demo_17_deinit(const StateGL* const sgl) {
	const SceneState* const scene_state = (SceneState*) sgl -> any_data;

	deinit_texture(scene_state -> lightmap_texture);
	deinit_weapon_sprite(&scene_state -> weapon_sprite);

	deinit_batch_draw_context(&scene_state -> sector_draw_context);
	deinit_batch_draw_context(&scene_state -> billboard_draw_context);

	deinit_list(scene_state -> sectors);
	deinit_list(scene_state -> billboard_animations);
	deinit_list(scene_state -> billboard_animation_instances);

	deinit_skybox(scene_state -> skybox);

	free(sgl -> any_data);

	deinit_demo_vars(sgl);
}

#ifdef DEMO_17
int main(void) {
	make_application(demo_17_drawer, demo_17_init, demo_17_deinit);
}
#endif
