#include "dda.h"

const float
	half_fov = fov / 2.0,
	theta_step = (float) fov / screen_width;

const float to_radians(const float degrees) {
	return degrees * M_PI / 180.0;
}

void dda(const float rotation, const float x, const float y) {
	// const theta step = bad

	unsigned long steps = -1;

	for (float theta = rotation - half_fov; theta < rotation + half_fov; theta += theta_step) {
		float rad_theta = to_radians(theta), dist = 0;
		const float cos_theta = cos(rad_theta), sin_theta = sin(rad_theta);

		while (dist += 0.01, steps++) {
			const double
				new_x = cos_theta * dist + x,
				new_y = sin_theta * dist + y;

			const byte point = map[(int) new_y][(int) new_x];
			if (point) {
				printf("Hit at x = %f and y = %f\n", new_x, new_y);
				break;
			}
		}
	}
	printf("Steps: %ld\n", steps);
}

int main() {
	dda(60.0, 2.0, 2.0);
}
