#include "../utils.c"
#include "../skybox.c"
#include "../data/maps.c"

#include "../sector.c"
#include "../billboard.c"
#include "../camera.c"

/*
- NEXT: final touches on new_map + a texmap for it
- NEXT 2: a sector BVH, through metasector trees, also called binary r-trees (alloc through node pool)
- NEXT 3: entities that don't turn to face the player (just static ones); defined by center, size, and normal
- NEXT 4: fix movement physics (one example: at FPS 10, can't jump over a block) (also, both pace and movement are stuttery - framerate spikes)
- NEXT 5: up-and-down moving platforms that can also work as doors (continually up-and-down moving, down if player close, or down if action fulfilled)
- NEXT 6: base fov on movement speed
- NEXT 7: deprecate most of StateGL's members and rely solely on vertex_array and any_data
- NEXT 8: pass in world size to sector shader

- Perlin noise-based lighting in 3D
- A map maker. An init json file that specifies textures and dimensions; draw/erase modes, export, and choose heights and textures
- More efficiently set statemap bit ranges, maybe
- Camera var names to yaw, pitch, and roll (maybe)
- Billboard lighting that matches the sector lighting
- Base darkest distance of attenuated light on the world size
- Can't use red cross for health since it's copyrighted

 - Crouch
- Accelerate through pressing a key
- Clipping before hitting walls head-on
- Tilt when turning

- Make deceleration framerate-independent
- Sometimes, jumping and landing on a higher surface is a bit jerky
- Animations go slower at 5 FPS
- And the red area is still slow

- Blit 2D sprite to whole screen
- Blit color rect to screen
- Then, flat weapon (that comes after physics)
- Use more of the cglm functions in `update_camera`, or make my own

- In the end, 5 shaders + accel components: sectors, billboards, skybox, weapon, ui elements
*/

typedef struct {
	const GLuint lightmap_texture; // This is grayscale

	List sectors;
	BatchDrawContext sector_draw_context;

	const List animations, animation_instances;
	BatchDrawContext billboard_draw_context;

	const Skybox skybox;
	byte* const heightmap;
	const byte map_size[2];
} SceneState;

StateGL demo_17_init(void) {
	StateGL sgl = {.vertex_array = init_vao(), .num_vertex_buffers = 0, .num_textures = 0};

	SceneState scene_state = {
		// "../assets/palace_perlin.bmp", "../assets/water_grayscale.bmp"
		.lightmap_texture = init_plain_texture("../assets/palace_perlin.bmp", TexPlain, TexNonRepeating, OPENGL_GRAYSCALE_INTERNAL_PIXEL_FORMAT),

		.animations = LIST_INITIALIZER(animation) (4,
			(Animation) {.texture_id_range = {2, 47}, .secs_per_frame = 0.02f}, // Flying carpet
			(Animation) {.texture_id_range = {48, 52}, .secs_per_frame = 0.15f}, // Torch
			(Animation) {.texture_id_range = {61, 63}, .secs_per_frame = 0.08f}, // Eddie, attacking
			(Animation) {.texture_id_range = {76, 79}, .secs_per_frame = 0.07f} // Trooper, idle
		),

		.animation_instances = LIST_INITIALIZER(animation_instance) (6,
			(AnimationInstance) {.ids = {.billboard = 4, .animation = 0}, .last_frame_time = 0.0f}, // Flying carpet
			(AnimationInstance) {.ids = {.billboard = 5, .animation = 1}, .last_frame_time = 0.0f}, // Torch

			(AnimationInstance) {.ids = {.billboard = 6, .animation = 2}, .last_frame_time = 0.0f}, // Eddies
			(AnimationInstance) {.ids = {.billboard = 7, .animation = 2}, .last_frame_time = 0.0f},

			(AnimationInstance) {.ids = {.billboard = 8, .animation = 3}, .last_frame_time = 0.0f}, // Troopers
			(AnimationInstance) {.ids = {.billboard = 9, .animation = 3}, .last_frame_time = 0.0f}
		),

		.skybox = init_skybox("../assets/mountain_2.bmp"),
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
		// New + Checker:
		// 1, 0, 128, 128, "../../../../assets/walls/pyramid_bricks_4.bmp"

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

	if (first_call) { // start new map: 1.5f, 0.5f, 1.5f
		init_camera(&camera, (vec3) {3.9f, 0.5f, 6.0f}); // 5.0f. 0.5f, 5.0f
		physics_obj.heightmap = scene_state -> heightmap;
		physics_obj.map_size[0] = scene_state -> map_size[0];
		physics_obj.map_size[1] = scene_state -> map_size[1];
		first_call = 0;
	}

	update_animation_instances(
		&scene_state -> animation_instances,
		&scene_state -> animations,
		&scene_state -> billboard_draw_context.buffers.cpu);

	update_camera(&camera, get_next_event(), &physics_obj);

	draw_visible_sectors(&scene_state -> sector_draw_context, &scene_state -> sectors,
		&camera, scene_state -> lightmap_texture, scene_state -> map_size);
	// Skybox after sectors b/c most skybox fragments would be unnecessarily drawn otherwise

	draw_skybox(scene_state -> skybox, &camera);
	draw_visible_billboards(&scene_state -> billboard_draw_context, &camera);
}

void demo_17_deinit(const StateGL* const sgl) {
	const SceneState* const scene_state = (SceneState*) sgl -> any_data;

	deinit_list(scene_state -> sectors);
	deinit_batch_draw_context(&scene_state -> sector_draw_context);

	deinit_list(scene_state -> animations);
	deinit_list(scene_state -> animation_instances);
	deinit_batch_draw_context(&scene_state -> billboard_draw_context);

	deinit_texture(scene_state -> lightmap_texture);
	deinit_skybox(scene_state -> skybox);
	free(sgl -> any_data);

	deinit_demo_vars(sgl);
}

#ifdef DEMO_17
int main(void) {
	make_application(demo_17_drawer, demo_17_init, demo_17_deinit);
}
#endif
