// structs: triangle, circle

typedef struct {
	const vec center;
	const double radius;
} Circle;

typedef double Triangle[3][2];

inlinable double diffuse_circle(const vec pos, const Circle circle) {
	const vec center_diff = pos - circle.center;
	const double radius_squared = circle.radius * circle.radius;
	const double d_squared = center_diff[0] * center_diff[0] + center_diff[1] * center_diff[1];
	return (d_squared <= radius_squared) ? radius_squared - d_squared : 0.0;
}

// https://stackoverflow.com/questions/2049582/how-to-determine-if-a-point-is-in-a-2d-triangle
inlinable double line_side(const vec p1, const double p2[2], const double p3[2]) {
	return (p1[0] - p3[0]) * (p2[1] - p3[1]) - (p2[0] - p3[0]) * (p1[1] - p3[1]);
}

inlinable byte flat_triangle(const vec pos, const Triangle triangle) {
	const double
		slope_1 = line_side(pos, triangle[0], triangle[1]),
		slope_2 = line_side(pos, triangle[1], triangle[2]),
		slope_3 = line_side(pos, triangle[2], triangle[0]);

	const byte
		has_neg = (slope_1 < 0) || (slope_2 < 0) || (slope_3 < 0),
		has_pos = (slope_1 > 0) || (slope_2 > 0) || (slope_3 > 0);

	return !(has_neg && has_pos);
}

/* Shading explained:
The `SHADING_ENABLED` flag is mostly for debugging (full visibility).
For a given wall, if it is far away, its height will be small.
Therefore, if a wall's height is small, it should be rendererd darker.
Dividing the wall height by the screen height gives an accurate darkness ratio.
Depending on the object's position (a floor pixel, or a wall/sprite column),
it may be considered darker or lighter (a dark corridor vs. a light plaza),
and that is what `current_level.shader` calculates. That shader may call sub-shading
functions like `diffuse_circle` above. */

#ifdef SHADING_ENABLED

inlinable double calculate_shade(const double wall_h, const vec pos) {
	double shade = wall_h / settings.screen_height * current_level.shader(pos);
	if (shade < current_level.darkest_shade) shade = current_level.darkest_shade;
	return shade > 1.0 ? 1.0 : shade;
}

#else

#define calculate_shade(a, b) 1.0

#endif
