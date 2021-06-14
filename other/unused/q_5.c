#include <SDL2/SDL.h>
#include <math.h>

#define SET_COLOR(r, g, b) SDL_SetRenderDrawColor(renderer, r, g, b, SDL_ALPHA_OPAQUE)

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
	theta_step = 0.05,
	dist_step = 0.1,
	width_ratio = (float) screen_width / map_width,
	height_ratio = (float) screen_height / map_height;

const unsigned char map[map_height][map_width] = {
	{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
	{1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1},
	{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
	{1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1},
	{1, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 1},
	{1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1},
	{1, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1},
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

void draw_rectangle(SDL_Rect rectangle, int r, int g, int b) {
	SET_COLOR(r, g, b);
	SDL_RenderFillRect(renderer, &rectangle);
	SDL_RenderDrawRect(renderer, &rectangle);
}

void raycast(Player player) {
	SET_COLOR(210, 180, 140);

	const float
		half_fov = player.fov / 2,
		rel_x = player.x * width_ratio, rel_y = player.y * height_ratio;

	float screen_x = 0, step_x = (screen_width / player.fov) * theta_step;

	for (float theta = player.angle - half_fov; theta < player.angle + half_fov; theta += theta_step) {
		const float rad_theta = to_radians(theta);
		const float cos_theta = cos(rad_theta), sin_theta = sin(rad_theta);

		float dist = 0;
		while (dist += dist_step) {
			const float
				new_x = cos_theta * dist + rel_x,
				new_y = sin_theta * dist + rel_y;

			if (map[(int) (new_y / height_ratio)][(int) (new_x / width_ratio)]) {
				dist *= cos(to_radians(theta - player.angle));
				float double_dist = 2 * dist;
				if (double_dist >= screen_height) break;
				SDL_Rect column = {screen_x, dist, step_x + 1, screen_height - double_dist};

				SDL_RenderFillRect(renderer, &column);
				SDL_RenderDrawRect(renderer, &column);
				break;
			}
		}
		screen_x += step_x;
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

	Player player = {5, 5, 0, 0, 0, 60};
	SDL_Rect the_ceiling = {0, 0, screen_width, screen_height / 2};
	SDL_Rect the_floor = {0, screen_height / 2, screen_width, screen_height};
	const Uint8* keys = SDL_GetKeyboardState(NULL);

	while (1) {
		handle_input(keys, &player);

		draw_rectangle(the_ceiling, 96, 96, 96);
		draw_rectangle(the_floor, 255,69,0);

		raycast(player);

		SDL_RenderPresent(renderer);
		SDL_UpdateWindowSurface(window);
	}
}