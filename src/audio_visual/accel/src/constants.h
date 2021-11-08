/*
Max world size = 255 by 255 by 255 (with top left corner of block as origin).
So, max look distance in world = sqrt(255 * 255 + 255 * 255 + 255 * 255),
which equals 441.6729559300637
*/

const struct {
	const GLfloat near, far;
} clip_dists = {0.1f, 441.6729559300637f};
