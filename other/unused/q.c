enum {
	screen_height = 500, screen_width = 1000,
	map_height = 6, map_width = 10,
};

const float width_ratio = (float) screen_width / map_width,
	height_ratio = (float) screen_height / map_height;

const unsigned char map[map_height][map_width] = {
	{1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
	{1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
	{1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
	{1, 0, 0, 1, 1, 1, 1, 0, 0, 1},
	{1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
	{1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
};

typedef struct {
	float angle, fov;
} Camera;

typedef struct {
	float x, y;
	Camera camera;
} Player;

#define SET_COLOR(rend, r, g, b) SDL_SetRenderDrawColor(rend, r, g, b, SDL_ALPHA_OPAQUE)

#include <SDL2/SDL.h>
#include <math.h>

SDL_Window* window;
SDL_Renderer* renderer;

void render(Player player) {
	int rel_x = player.x * width_ratio, rel_y = player.y * height_ratio;
	Camera cam = player.camera;
	float half_fov = cam.fov / 2, width_fov_ratio = screen_width / cam.fov;
	
	for (float theta = cam.angle - half_fov, vline_x = 0;
		theta < cam.angle + half_fov;
		theta += 0.5, vline_x += width_fov_ratio) {

		float radian_theta = theta * (M_PI / 180);
		float cos_theta = cos(radian_theta), sin_theta = sin(radian_theta);

		float d = 0, new_x, new_y;
		while (d += 0.5) {
			new_y = sin_theta * d + rel_y, new_x = cos_theta * d + rel_x;

			int scaled_down_y = round(new_y / height_ratio),
				scaled_down_x = round(new_x / width_ratio);

			if (map[scaled_down_y][scaled_down_x]) {
				float dist_to_wall = sqrt(pow(new_x - rel_x, 2) + pow(new_y - rel_y, 2));

				if (dist_to_wall * 2 >= screen_height)
					break;

				SDL_Rect r;
				r.x = vline_x;
				r.y = dist_to_wall;
				r.w = width_fov_ratio * 2;
				r.h = screen_height - dist_to_wall * 2;

				SET_COLOR(renderer, 0, 191, 255);
				SDL_RenderFillRect(renderer, &r);
				SDL_RenderDrawRect(renderer, &r);
				break;
			}
		}
	}
}

int main() {
	SDL_CreateWindowAndRenderer(screen_width, screen_height, 0, &window, &renderer);
	SDL_SetWindowTitle(window, "Raycaster");
	Player player = {2, 2, {45, 90}};

	while (1) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_QUIT:
					SDL_DestroyWindow(window), SDL_DestroyRenderer(renderer);
					return 0;
				case SDL_KEYDOWN: {
					float radian_angle = player.camera.angle * (M_PI / 180);
					float move_x = cos(radian_angle) * 0.1, move_y = sin(radian_angle) * 0.1;
					switch (event.key.keysym.sym) {
						case SDLK_UP: player.x += move_x, player.y += move_y; break;
						case SDLK_DOWN: player.x -= move_x, player.y -= move_y; break;
						case SDLK_LEFT: player.camera.angle -= 2; break;
						case SDLK_RIGHT: player.camera.angle += 2; break;
					}
					if (player.x < 0) player.x = 0;
					else if (player.x > screen_width) player.x = screen_width;

					if (player.y < 0) player.y = 0;
					else if (player.y > screen_height) player.y = screen_height;

					if (player.camera.angle > 360) player.camera.angle = 0;
					else if (player.camera.angle < 0) player.camera.angle = 360;
				}
			}
		}
		SET_COLOR(renderer, 0, 0, 0);
		SDL_RenderClear(renderer);
		render(player);
		SDL_RenderPresent(renderer);
		SDL_UpdateWindowSurface(window);
	}
}