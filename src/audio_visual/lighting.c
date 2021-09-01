typedef double Triangle[3][2];

typedef struct {
	const vec center;
	const double radius;
} Circle;

inlinable double diffuse_circle(const vec pos, const Circle circle) {
	const vec center_diff = pos - circle.center;
	const double
		radius_squared = circle.radius * circle.radius,
		d_squared = center_diff[0] * center_diff[0] + center_diff[1] * center_diff[1];
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

//////////

const byte lightmap_samples_per_tile = 10;
const double lightmap_shader_downscaler = 0.1;

Lightmap init_lightmap(void) {
	Lightmap lightmap = {
		.size = (ivec) {
			current_level.map_size.x * lightmap_samples_per_tile,
			current_level.map_size.y * lightmap_samples_per_tile
		}
	};

	lightmap.data = wmalloc(lightmap.size.x * lightmap.size.y * sizeof(byte));

	vec pos;
	for (int y = 0; y < lightmap.size.y; y++) {
		pos[1] = (double) y / lightmap_samples_per_tile;
		for (int x = 0; x < lightmap.size.x; x++) {
			pos[0] = (double) x / lightmap_samples_per_tile;
			const double light = current_level.shader(pos) * lightmap_shader_downscaler;
			lightmap.data[y * lightmap.size.x + x] = ((light > 1.0) ? 1.0 : light) * 255;
		}
	}

	return lightmap;
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

double calculate_shade(const double wall_h, vec pos) {
	(void) wall_h;

	static const double small = 0.000000001, one_over_255 = 1.0 / 255.0;

	const Lightmap lightmap = current_level.lightmap;
	const vec lightmap_pos = (pos - vec_fill(small)) * vec_fill(lightmap_samples_per_tile);

	return lightmap.data[(int) lightmap_pos[1] * lightmap.size.x + (int) lightmap_pos[0]] * one_over_255;

	/*
	double shade = wall_h / settings.screen_height * current_level.shader(pos);
	if (shade < current_level.darkest_shade) shade = current_level.darkest_shade;
	return shade > 1.0 ? 1.0 : shade;
	*/
}

#else

#define calculate_shade(a, b) 1.0

#endif

/*
void lightmap_test(void) {
	SDL_Surface* const test = SDL_CreateRGBSurfaceWithFormat(
		0, current_level.map_size.x, current_level.map_size.y, 32, PIXEL_FORMAT);

	Uint32* read_surface_pixel(const SDL_Surface* const, const int, const int, const int);

	for (int y = 0; y < current_level.map_size.y; y++) {
		for (int x = 0; x < current_level.map_size.x; x++) {
			const double light_val = calculate_shade(0.0, (vec) {x, y});
			*read_surface_pixel(test, x, y, test -> format -> BytesPerPixel) =
				SDL_MapRGBA(test -> format, light_val * 255, light_val * 255, light_val * 255, 255);
		}
	}

	SDL_SaveBMP(test, "lightmap_sample.bmp");
	SDL_FreeSurface(test);
}
*/
