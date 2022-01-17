#include <SDL2/SDL.h>

#define PIXEL_FORMAT SDL_PIXELFORMAT_INDEX8
#define DEBUG(var, format) printf(#var " = %" #format "\n", var)

typedef float vec2[2];
typedef float mat4[4][4];

//////////

const int first_octave = 3, octaves = 50;

const float
	inner_rand_multiplier = 1.43f, outer_rand_multiplier = 2.0f, rand_subtrahend = 1.0f,
	persistence = 0.65f, scale = 1.0f, result_addend = 0.4f;

//////////

float dot(const vec2 a, const vec2 b) {
	return a[0] * b[0] + a[1] * b[1];
}

float fract(const float a) {
	return a - floorf(a);
}

float noise(const int x, const int y) {
	return outer_rand_multiplier
		* fract(sinf(inner_rand_multiplier * dot((vec2) {x, y}, (vec2) {12.9898f, 78.233f})) * 43758.5453f)
		- rand_subtrahend;
}

float lerp(const float a, const float b, const float weight) {
	return a * (1.0f - weight) + b * weight;
}

float noise_from_samples(const int cx, const int cy, const mat4 samples) {
	return samples[cy][cx] * 0.25f
		+ (samples[cy - 1][cx] + samples[cy + 1][cx] + samples[cy][cx - 1] + samples[cy][cx + 1]) * 0.125f
		+ (samples[cy - 1][cx - 1] + samples[cy - 1][cx + 1] + samples[cy + 1][cx - 1] + samples[cy + 1][cx + 1]) * 0.0625f;
}

float lerp_noise(const vec2 pos) {
	const int ix = pos[0], iy = pos[1];

	const mat4 samples = {
		{noise(ix - 1, iy - 1), noise(ix, iy - 1), noise(ix + 1, iy - 1), noise(ix + 2, iy - 1)},
		{noise(ix - 1, iy),     noise(ix, iy),     noise(ix + 1, iy),     noise(ix + 2, iy)},
		{noise(ix - 1, iy + 1), noise(ix, iy + 1), noise(ix + 1, iy + 1), noise(ix + 2, iy + 1)},
		{noise(ix - 1, iy + 2), noise(ix, iy + 2), noise(ix + 1, iy + 2), noise(ix + 2, iy + 2)}
	};

	const float fract_x = fract(pos[0]);

	const float
		noise_top = lerp(noise_from_samples(1, 1, samples), noise_from_samples(2, 1, samples), fract_x),
		noise_bottom = lerp(noise_from_samples(1, 2, samples), noise_from_samples(2, 2, samples), fract_x);

	return lerp(noise_top, noise_bottom, fract(pos[1]));
}

float perlin_2D(const vec2 pos) {
	float noise = result_addend, amplitude = powf(persistence, first_octave);
	int frequency = 2 << (first_octave - 1);

	// Fractal brownian motion
	for (int i = first_octave; i < octaves + first_octave; i++, frequency <<= 1, amplitude *= persistence)
		noise += lerp_noise((vec2) {pos[0] * frequency, pos[1] * frequency}) * amplitude;
	if (noise > 1.0f) return 1.0f;
	return (noise < 0.0f) ? 0.0f : noise;
}

//////////

SDL_Surface* make_perlin_map(const int size[2]) {
	SDL_Surface* const perlin_map = SDL_CreateRGBSurfaceWithFormat(0,
		size[0], size[1], SDL_BITSPERPIXEL(PIXEL_FORMAT), PIXEL_FORMAT);

	SDL_LockSurface(perlin_map);

	enum {num_colors = 256};
	SDL_Color palette[num_colors];
	for (int i = 0; i < num_colors; i++) palette[i] = (SDL_Color) {i, i, i, 255};
	SDL_SetPaletteColors(perlin_map -> format -> palette, palette, 0, num_colors);

	const vec2 downscale_vals = {1.0f / size[0] * scale, 1.0f / size[1] * scale};

	const int bytes_per_row = perlin_map -> pitch;
	Uint8* index_row = perlin_map -> pixels;

	for (int y = 0; y < size[1]; y++, index_row += bytes_per_row) {
		const float downscaled_y = y * downscale_vals[1];
		for (int x = 0; x < size[0]; x++)
			index_row[x] = perlin_2D((vec2) {x * downscale_vals[0], downscaled_y}) * num_colors;
	}

	SDL_UnlockSurface(perlin_map);
	return perlin_map;
}

/*
- This should take command line arguments later
- 2D first, 3D later
- Compile this with the map code once done
*/

int main(void) {
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		fprintf(stderr, "Could not launch SDL: '%s'\n", SDL_GetError());
		return 1;
	}

	SDL_Surface* const perlin_map = make_perlin_map((int[2]) {256, 256});
	SDL_SaveBMP(perlin_map, "out.bmp");
	SDL_FreeSurface(perlin_map);

	SDL_Quit();
}
