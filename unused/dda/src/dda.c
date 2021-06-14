#include "dda.h"

typedef struct {
	float x, y;
} Vector;

const Vector get_direction(const float theta) {
	const Vector dir = {cos(theta), sin(theta)};
	return dir;
}

void dda(const Vector pos, const Vector dir) {
	const float
		normalized_dx = 1.0 / dir.x,
		normalized_dy = 1.0 / dir.y;

	const int
		dir_x_positive = dir.x > 0.0,
		dir_y_positive = dir.y > 0.0;

	const int
		step_x = dir_x_positive ? 1 : -1,
		step_y = dir_y_positive ? 1 : -1;

	float
		next_x = (dir_x_positive ? floor(pos.x) : ceil(pos.x)) + step_x,
		next_y = (dir_y_positive ? floor(pos.y) : ceil(pos.y)) + step_y,
		dist = 0.0;

	while (1) {
		printf("Going\n");
		const float
			cast_x = dir.x * dist + pos.x,
			cast_y = dir.y * dist + pos.y;

		if (map[(int) cast_y][(int) cast_x]) {
			printf("Wall at %lf, %lf\n", cast_x, cast_y);
			break;
		}

		const float
			dx = normalized_dx * (next_x - pos.x),
			dy = normalized_dy * (next_y - pos.y);

		dist = dx < dy ? dx : dy;
		if (dx == dist) next_x += step_x;
		if (dy == dist) next_y += step_y;
	}
}

int main() {
	const Vector
		pos = {2.0, 2.0},
		dir = get_direction(1.5);

	dda(pos, dir);
}
