#include <SDL2/SDL.h>

#define PIXEL_FORMAT SDL_PIXELFORMAT_INDEX8
#define DEBUG(var, format) printf(#var " = %" #format "\n", var)

typedef uint64_t integer_t;
typedef double floating_t;

typedef floating_t vec2[2];
typedef floating_t mat4[4][4];

//////////

const integer_t first_octave = 3ul, octaves = 60ull;

const floating_t
	inner_rand_multiplier = 1.64, outer_rand_multiplier = 2.0, rand_subtrahend = 1.0,
	persistence = 0.6, scale = 1.0, result_addend = 0.3;

//////////

floating_t dot(const vec2 a, const vec2 b) {
	return a[0] * b[0] + a[1] * b[1];
}

floating_t fract(const floating_t a) {
	return a - floor(a);
}

floating_t noise(const integer_t x, const integer_t y) {
	return outer_rand_multiplier
		* fract(sin(inner_rand_multiplier * dot((vec2) {x, y}, (vec2) {12.9898, 78.233})) * 43758.5453)
		- rand_subtrahend;
}

floating_t lerp(const floating_t a, const floating_t b, const floating_t weight) {
	return a * (1.0 - weight) + b * weight;
}

floating_t noise_from_samples(const integer_t cx, const integer_t cy, const mat4 samples) {
	return samples[cy][cx] * 0.25
		+ (samples[cy - 1ull][cx] + samples[cy + 1ull][cx] + samples[cy][cx - 1ull] + samples[cy][cx + 1ull]) * 0.125
		+ (samples[cy - 1ull][cx - 1ull] + samples[cy - 1ull][cx + 1ull] + samples[cy + 1ull][cx - 1ull] + samples[cy + 1ull][cx + 1ull]) * 0.0625;
}

floating_t lerp_noise(const vec2 pos) {
	const integer_t ix = pos[0], iy = pos[1];

	const mat4 samples = {
		{noise(ix - 1ull, iy - 1ull), noise(ix, iy - 1ull), noise(ix + 1ull, iy - 1ull), noise(ix + 2ull, iy - 1ull)},
		{noise(ix - 1ull, iy),        noise(ix, iy),        noise(ix + 1ull, iy),        noise(ix + 2ull, iy)},
		{noise(ix - 1ull, iy + 1ull), noise(ix, iy + 1ull), noise(ix + 1ull, iy + 1ull), noise(ix + 2ull, iy + 1ull)},
		{noise(ix - 1ull, iy + 2ull), noise(ix, iy + 2ull), noise(ix + 1ull, iy + 2ull), noise(ix + 2ull, iy + 2ull)}
	};

	const floating_t fract_x = fract(pos[0]);

	const floating_t
		noise_top = lerp(noise_from_samples(1ull, 1ull, samples), noise_from_samples(2ull, 1ull, samples), fract_x),
		noise_bottom = lerp(noise_from_samples(1ull, 2ull, samples), noise_from_samples(2ull, 2ull, samples), fract_x);

	return lerp(noise_top, noise_bottom, fract(pos[1]));
}

floating_t perlin_2D(const vec2 pos) {
	floating_t noise = result_addend, amplitude = pow(persistence, first_octave);
	uint64_t frequency = 2ull << (first_octave - 1ull);

	// Fractal brownian motion
	for (integer_t i = first_octave; i < octaves + first_octave; i++, frequency <<= 1ull, amplitude *= persistence)
		noise += lerp_noise((vec2) {pos[0] * frequency, pos[1] * frequency}) * amplitude;

	if (noise > 1.0) return 1.0;
	return (noise < 0.0) ? 0.0 : noise;
}

//////////

SDL_Surface* make_perlin_map(const integer_t size[2]) {
	SDL_Surface* const perlin_map = SDL_CreateRGBSurfaceWithFormat(0,
		size[0], size[1], SDL_BITSPERPIXEL(PIXEL_FORMAT), PIXEL_FORMAT);

	SDL_LockSurface(perlin_map);

	enum {num_colors = 256};
	SDL_Color palette[num_colors];
	for (integer_t i = 0; i < num_colors; i++) palette[i] = (SDL_Color) {i, i, i, 255};
	SDL_SetPaletteColors(perlin_map -> format -> palette, palette, 0, num_colors);

	const vec2 downscale_vals = {1.0 / size[0] * scale, 1.0 / size[1] * scale};

	const int bytes_per_row = perlin_map -> pitch;
	Uint8* index_row = perlin_map -> pixels;

	for (integer_t y = 0ull; y < size[1]; y++, index_row += bytes_per_row) {
		const floating_t downscaled_y = y * downscale_vals[1];
		for (integer_t x = 0ull; x < size[0]; x++)
			index_row[x] = perlin_2D((vec2) {x * downscale_vals[0], downscaled_y}) * num_colors;
	}

	SDL_UnlockSurface(perlin_map);
	return perlin_map;
}

/*
- This should take command line arguments later
- 3D: http://www.science-and-fiction.org/rendering/noise.html
- Compile this with the map code once done
*/

int main(void) {
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		fprintf(stderr, "Could not launch SDL: '%s'\n", SDL_GetError());
		return 1;
	}

	SDL_Surface* const perlin_map = make_perlin_map((integer_t[2]) {256ull, 256ull});
	SDL_SaveBMP(perlin_map, "out.bmp");
	SDL_FreeSurface(perlin_map);

	SDL_Quit();
}
