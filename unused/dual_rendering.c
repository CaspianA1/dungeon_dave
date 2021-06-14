/*
typedef struct {
	union {
		Sprite* sprite;
		Animation* animation;
	};
	int is_sprite;
} Renderable;

#define animation_count 4

void draw_renderable(const Player player, Animation* animations) {
	const static int renderable_count = sprite_count + animation_count;
	Renderable renderable[renderable_count];

	for (int i = 0; i < renderable_count; i++) {
		Renderable* r = &renderable[i];
		r -> is_sprite = i < sprite_count;
		if (i < sprite_count) {
			r -> sprite = &sprites[i];
		}
		else {
			r -> animation = &animations[i - sprite_count];
		}
	}
}

void dual_render_test(const Player player) {
	const int animation_fps = 12;

	Animation animations[animation_count] = {
		init_animation("../assets/spritesheets/bogo.bmp", 2, 3, 6, animation_fps),
		init_animation("../assets/spritesheets/carrot.bmp", 5, 3, 12, animation_fps),
		init_animation("../assets/spritesheets/robot.bmp", 2, 2, 3, animation_fps),
		init_animation("../assets/spritesheets/numbers.bmp", 3, 3, 8, animation_fps)
	};

	draw_renderable(player, animations);

	for (int i = 0; i < animation_count; i++)
		deinit_animation(animations[i]);
}
*/
