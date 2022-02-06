#include <SDL2/SDL.h>

#define WINDOW_W 800
#define WINDOW_H 600

#define DEBUG(var, format) printf(#var " = %" #format "\n", var)
#define DEBUG_FRECT(frect) printf(#frect " = {%lf, %lf, %lf, %lf}\n",\
	(double) frect.x, (double) frect.y, (double) frect.w, (double) frect.h)

//////////

typedef uint8_t byte;
typedef float vec2[2];

typedef struct {
	vec2 top_left, bottom_right;
} aabb;

//////////

enum {map_width = 40, map_height = 40};

const byte heightmap[map_height][map_width] = {
	{3, 3, 3, 3, 3, 3, 3, 3, 3, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5},
	{3, 0, 0, 3, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10,10,10,10,10,0, 0, 0, 5},
	{3, 0, 0, 3, 0, 0, 3, 0, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 10,0, 10,10,10,10,10,0, 0, 0, 5},
	{3, 0, 0, 0, 0, 0, 3, 0, 6, 8, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 6, 10,0, 10,10,10,10,10,0, 0, 0, 5},
	{3, 3, 3, 3, 0, 0, 3, 0, 6, 8, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 6, 10,0, 10,10,10,10,10,0, 0, 0, 5},
	{3, 0, 0, 0, 0, 0, 3, 0, 8, 8, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 6, 10,0, 10,10,10,10,10,0, 0, 0, 5},
	{5, 0, 0, 0, 0, 0, 5, 0, 8, 8, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 8, 1, 1, 1, 1, 1, 1, 6, 10,0, 10,10,10,10,10,0, 0, 0, 5},
	{5, 0, 0, 0, 0, 0, 5, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 8, 1, 1, 1, 1, 1, 1, 1, 6, 10,0, 10,10,10,10,10,0, 0, 0, 5},
	{5, 0, 0, 0, 0, 0, 5, 0, 8, 8, 1, 1, 1, 1, 1, 1, 1, 1, 1, 8, 1, 1, 1, 1, 1, 1, 1, 1, 6, 10,0, 10,10,10,10,10,0, 0, 0, 5},
	{5, 1, 1, 1, 1, 5, 5, 0, 8, 8, 1, 1, 1, 1, 1, 1, 1, 1, 8, 1, 1, 1, 1, 1, 1, 1, 1, 1, 6, 10,0, 10,10,10,10,10,0, 0, 0, 5},
	{6, 2, 2, 2, 2, 5, 5, 0, 6, 8, 2, 2, 2, 2, 2, 2, 2, 8, 2, 2, 2, 2, 8, 2, 2, 2, 2, 2, 6, 10,0, 10,10,10,10,10,1, 1, 1, 5},
	{6, 3, 3, 3, 3, 5, 5, 0, 6, 8, 3, 3, 3, 3, 3, 3, 8, 3, 3, 3, 3, 8, 2, 2, 2, 2, 2, 2, 6, 10,0, 10,10,10,10,10,1, 1, 1, 5},
	{6, 3, 3, 3, 3, 0, 0, 0, 6, 8, 8, 3, 3, 3, 3, 8, 3, 3, 3, 3, 8, 3, 2, 2, 2, 2, 2, 2, 6, 10,0, 0, 0, 0, 0, 0, 0, 0, 0, 5},
	{6, 3, 3, 3, 3, 6, 6, 6, 6, 6, 8, 8, 8, 8, 8, 3, 3, 3, 3, 8, 3, 3, 2, 2, 2, 2, 2, 2, 6, 10,0, 0, 10,0, 10,0, 10,0, 10,5},
	{6, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 8, 3, 3, 3, 2, 2, 2, 2, 2, 2, 6, 10,0, 0, 0, 0, 0, 0, 0, 0, 0, 5},
	{6, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 8, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 6, 10,0, 0, 10,0, 10,0, 10,0, 10,5},
	{6, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 8, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 6, 10,0, 0, 0, 0, 0, 0, 0, 0, 0, 5},
	{6, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 8, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 6, 10,0, 10,10,10,10,10,10,10,0, 5},
	{6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 8, 8, 8, 8, 8, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 6, 10,0, 0, 0, 0, 0, 0, 0, 0, 0, 5},
	{5, 2, 2, 2, 2, 2, 2, 2, 2, 6, 8, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 6, 10,0, 0, 0, 0, 0, 10,0, 10,10,5},
	{5, 2, 1, 1, 1, 1, 1, 1, 2, 6, 8, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 6, 10,0, 10,0, 10,10,0, 0, 0, 0, 5},
	{5, 2, 1, 0, 0, 0, 0, 1, 2, 6, 8, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 6, 10,0, 10,0, 10,0, 0, 0, 0, 0, 5},
	{5, 2, 1, 0, 0, 0, 0, 1, 2, 6, 8, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 6, 10,0, 10,0, 10,0, 0, 0, 0, 0, 5},
	{5, 2, 1, 0, 0, 0, 0, 1, 2, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 3, 3, 3, 2, 2, 2, 2, 2, 2, 6, 10,0, 0, 0, 0, 0, 0, 0, 0, 0, 5},
	{5, 2, 1, 0, 1, 1, 1, 1, 2, 0, 0, 0, 3, 3, 2, 2, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 6, 10,0, 0, 10,10, 0, 10,10,10, 0, 5},
	{5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 6, 10,0, 0, 0, 0, 0, 0, 0, 0, 0, 5},
	{5, 0, 0, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 0, 2, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 6, 10,0, 0, 0, 0, 0, 0, 0, 0, 0, 5},
	{5, 0, 0, 0, 3, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 6, 10,0, 0, 10, 10, 0, 0, 0, 0, 0, 5},
	{5, 0, 0, 0, 3, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 6, 10,0, 0, 10, 0, 0, 0, 0, 0, 0, 5},
	{5, 0, 0, 0, 3, 1, 1, 3, 3, 3, 3, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 6, 6, 6, 6, 6, 6, 10,0, 0, 10, 0, 10, 0, 0, 0, 0, 5},
	{5, 0, 0, 0, 3, 1, 1, 3, 3, 3, 3, 3, 3, 0, 0, 2, 0, 2, 0, 2, 0, 2, 2, 2, 2, 2, 2, 2, 2, 10,10, 10, 10, 0, 10, 0, 10, 0, 0, 5},
	{5, 0, 0, 0, 3, 2, 2, 3, 3, 3, 3, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 10,0, 0, 10, 0, 10, 0, 10, 0, 0, 5},
	{5, 1, 2, 3, 3, 2, 2, 3, 3, 3, 3, 3, 3, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,10, 10, 10, 10, 0, 0, 0, 0, 10, 0, 0, 0, 0, 5},
	{5, 1, 2, 3, 3, 2, 2, 3, 3, 3, 3, 3, 3, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 5},
	{5, 0, 0, 0, 3, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 5},
	{5, 0, 0, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 5},
	{5, 0, 0, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 5},
	{5, 0, 0, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 5},
	{5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 5},
	{10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10}
};

//////////

byte has_point_at(const float x, const float y) {
	if (x < 0.0f || y < 0.0f || x >= map_width || y >= map_height) return 1;
	return !!heightmap[(byte) y][(byte) x];
}

byte box_in_point(const vec2 pos, const vec2 half_border) {
	return has_point_at(pos[0] - half_border[0], pos[1] - half_border[1])
		|| has_point_at(pos[0] + half_border[0], pos[1] + half_border[1])
		|| has_point_at(pos[0] - half_border[0], pos[1] + half_border[1])
		|| has_point_at(pos[0] + half_border[0], pos[1] - half_border[1]);
}

void move(vec2 pos, const vec2 border_size, const Uint8* const keys) {
	const float step = 0.1f;

	const vec2 old_pos = {pos[0], pos[1]};

	const byte
		left = keys[SDL_SCANCODE_LEFT], right = keys[SDL_SCANCODE_RIGHT],
		up = keys[SDL_SCANCODE_UP], down = keys[SDL_SCANCODE_DOWN];
	
	const vec2 half_border = {border_size[0] * 0.5f, border_size[1] * 0.5f};
	
	if (left ^ right) {
		pos[0] += left ? -step : step;
		if (box_in_point(pos, half_border)) pos[0] = old_pos[0];
	}

	if (up ^ down) {
		pos[1] += up ? -step : step;
		if (box_in_point(pos, half_border)) pos[1] = old_pos[1];
	}
}

void render_map(SDL_Renderer* const renderer, const vec2 pos, const vec2 border_size) {
	static const vec2 block_screen_size = {
		(float) WINDOW_W / map_width, (float) WINDOW_H / map_height
	};

	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
	SDL_RenderClear(renderer);
	SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);

	for (byte y = 0; y < map_height; y++) {
		for (byte x = 0; x < map_width; x++) {
			if (heightmap[y][x] == 0) continue;

			const SDL_FRect tile = {x * block_screen_size[0], y * block_screen_size[1], block_screen_size[0], block_screen_size[1]};
			SDL_RenderDrawRectF(renderer, &tile);
		}
	}

	SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
	SDL_RenderDrawPointF(renderer, pos[0] * block_screen_size[0], pos[1] * block_screen_size[1]);

	const SDL_FRect border_world_space = {
		pos[0] - border_size[0] * 0.5f, pos[1] - border_size[1] * 0.5f, border_size[0], border_size[1]
	};

	const SDL_FRect border = {
		border_world_space.x * block_screen_size[0], border_world_space.y * block_screen_size[1], 
		border_world_space.w * block_screen_size[0], border_world_space.h * block_screen_size[1],
	};

	SDL_SetRenderDrawColor(renderer, 210, 180, 140, 255);
	SDL_RenderDrawRectF(renderer, &border);
}

//////////

int main(void) {
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
		fprintf(stderr, "Could not launch SDL: %s\n", SDL_GetError());
		return 1;
	}

	SDL_SetHintWithPriority(SDL_HINT_RENDER_VSYNC, "1", SDL_HINT_OVERRIDE);

	SDL_Window* window;
	SDL_Renderer* renderer;

	SDL_CreateWindowAndRenderer(WINDOW_W, WINDOW_H, 0, &window, &renderer);
	SDL_SetWindowTitle(window, "collision demo");

	vec2 pos = {1.5f, 1.5f};
	const vec2 border_size = {0.5f, 0.999f};

	const Uint8* const keys = SDL_GetKeyboardState(NULL);
	SDL_Event event;
	byte running = 1;

	while (running) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				running = 0;
				break;
			}
		}

		move(pos, border_size, keys);
		render_map(renderer, pos, border_size);
		SDL_RenderPresent(renderer);
	}

	SDL_DestroyWindow(window);
	SDL_DestroyRenderer(renderer);
	SDL_Quit();
}
