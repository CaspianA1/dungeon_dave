#include <SDL2/SDL.h>
#include <math.h>

typedef struct {
	float x, y, prev_x, prev_y, angle, fov;
} Player;

enum {
	map_width = 12, map_height = 15,
	screen_width = 800, screen_height = 500
};

const float
	move_speed_decr = 0.08,
	angle_turn = 2.0,
	ray_theta_step = 0.4,
	ray_dist_step = 0.8,
	darkening = 1.8,
	width_ratio = (float) screen_width / map_width,
	height_ratio = (float) screen_height / map_height;

const unsigned char map[map_height][map_width] = {
	{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
	{1, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 1},
	{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
	{1, 0, 0, 0, 0, 0, 0, 2, 1, 1, 1, 1},
	{1, 0, 4, 3, 2, 0, 0, 0, 2, 0, 0, 1},
	{1, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 1},
	{1, 0, 0, 0, 4, 3, 2, 1, 4, 0, 0, 1},
	{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
	{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
	{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
	{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
	{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
	{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
	{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
	{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
};

SDL_Window* window;
SDL_Renderer* renderer;

float to_radians(float degrees) {
	return degrees * (M_PI / 180.0f);
}

float distance(float x0, float y0, float x1, float y1) {
	return sqrt(((x1 - x0) * (x1 - x0)) + ((y1 - y0) * (y1 - y0)));
}

void shade(int* color, int darkener) {
	int darkened = *color - darkener;
	*color = darkened < 0 ? 0 : darkened;
}

void draw_rectangle(SDL_Rect rectangle, int r, int g, int b) {
	SDL_SetRenderDrawColor(renderer, r, g, b, SDL_ALPHA_OPAQUE);
	SDL_RenderFillRect(renderer, &rectangle);
	SDL_RenderDrawRect(renderer, &rectangle);
}

void raycast(Player player) {
	float relative_x = player.x * width_ratio;
	float relative_y = player.y * height_ratio;
	float half_fov = player.fov / 2;
	float width_fov_ratio = (screen_width / player.fov) / 2;

	// the core of my problem may be in the constant increment of my angle
	for (float theta = player.angle - half_fov, beta = -player.fov, screen_x = 0;
		theta < player.angle + half_fov;
		theta += ray_theta_step, beta += ray_theta_step * 2, screen_x += width_fov_ratio) {

		float adjust = cos(to_radians(theta - player.angle));

		float radian_theta = to_radians(theta);
		float cos_theta = cos(radian_theta), sin_theta = sin(radian_theta);

		float d = 0, new_x, new_y;
		while (d += ray_dist_step) {
			new_x = cos_theta * d + relative_x;
			new_y = sin_theta * d + relative_y;

			int map_x = new_x / width_ratio, map_y = new_y / height_ratio;
			int map_point = map[map_y][map_x];

			if (map_point) {
				int dist_wall = distance(relative_x, relative_y, new_x, new_y) * adjust;

				int twice_dist_wall = 2 * dist_wall;
				if (twice_dist_wall >= screen_height) break;
				else if (map_point) { // succeeds when a wall is present
					int r, g, b;
					switch (map_point) {
						case 1: r = 255, g = 255, b = 0; break;
						case 2: r = 0, g = 128, b = 128; break;
						case 3: r = 255, g = 165, b = 0; break;
						case 4: r = 255, g = 0, b = 0; break;
					}

					int color_decr = dist_wall / darkening;
					shade(&r, color_decr);
					shade(&g, color_decr);
					shade(&b, color_decr);

					SDL_Rect vertical_line = {
						screen_x, dist_wall,
						width_fov_ratio + 1,
						screen_height - twice_dist_wall
					};

					draw_rectangle(vertical_line, r, g, b);
					break;
				}
			}
		}
	}
}

void handle_input(const Uint8* keys, Player* player) {
	SDL_Event event;

	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT) {
			SDL_DestroyWindow(window);
			SDL_DestroyRenderer(renderer);
			exit(0);
		}

		else if (event.type == SDL_KEYDOWN) {
			float radian_theta = to_radians(player -> angle);
			float move_x = cos(radian_theta) * move_speed_decr,
				move_y = sin(radian_theta) * move_speed_decr;

			// handle arrow keys
			if (keys[SDL_SCANCODE_UP]) player -> x += move_x, player -> y += move_y;
			if (keys[SDL_SCANCODE_DOWN]) player -> x -= move_x, player -> y -= move_y;
			if (keys[SDL_SCANCODE_LEFT]) player -> x += move_y, player -> y -= move_x;
			if (keys[SDL_SCANCODE_RIGHT]) player -> x -= move_y, player -> y += move_x;

			// handle 'a' and 's' for angle changes
			if (keys[SDL_SCANCODE_A]) player -> angle -= angle_turn;
			if (keys[SDL_SCANCODE_S]) player -> angle += angle_turn;

			// safeguards for invalid positions and angles
			if (player -> x < 0) player -> x = 0;
			else if (player -> x > screen_width) player -> x = screen_width;

			if (player -> y < 0) player -> y = 0;
			else if (player -> y > screen_height) player -> y = screen_height;

			// move the player to their previous coordinate if they're in a wall
			if (map[(int) player -> y][(int) player -> x])
				player -> y = player -> prev_y, player -> x = player -> prev_x;

			if (player -> angle > 360) player -> angle = 0;
			else if (player -> angle < 0) player -> angle = 360;

			player -> prev_y = player -> y, player -> prev_x = player -> x;
		}
	}
}

int main() {
	SDL_CreateWindowAndRenderer(screen_width, screen_height, 0, &window, &renderer);
	SDL_SetWindowTitle(window, "Raycaster");	

	Player player = {5, 5, 0, 0, 90, 90};
	SDL_Rect the_ceiling = {0, 0, screen_width, screen_height / 2};
	SDL_Rect the_floor = {0, screen_height / 2, screen_width, screen_height};
	const Uint8* keys = SDL_GetKeyboardState(NULL);

	while (1) {
		handle_input(keys, &player);

		draw_rectangle(the_ceiling, 96, 96, 96);
		draw_rectangle(the_floor, 210, 180, 140);

		raycast(player);

		SDL_RenderPresent(renderer);
		SDL_UpdateWindowSurface(window);
	}
}