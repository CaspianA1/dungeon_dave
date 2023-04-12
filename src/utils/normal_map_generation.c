#include "utils/normal_map_generation.h"
#include "cglm/cglm.h" // For various cglm defs
#include "data/constants.h" // For `one_over_max_byte_value`, and `max_byte_value`
#include "utils/alloc.h" // For `alloc`, and `dealloc`
#include "utils/failure.h" // For `FAIL`
#include "utils/opengl_wrappers.h" // For various OpenGL wrappers

////////// This code concerns heightmap creation.

static void generate_heightmap(SDL_Surface* const src, SDL_Surface* const dest, const GLfloat heightmap_scale) {
	////////// Skipping heightmap generation if the heightmap scale is small enough

	/* Any heightmap scale smaller than this one will result in any
	pixel component being multiplied by it turning into zero */
	const GLfloat smallest_possible_heightmap_scale = constants.one_over_max_byte_value;

	if (heightmap_scale < smallest_possible_heightmap_scale) {
		// Getting the blank color explicitly, because who knows if the blank color doesn't map to 0?
		const Uint32 blank_color = SDL_MapRGB(dest -> format, 0, 0, 0);
		SDL_FillRect(dest, NULL, blank_color);
		return;
	}

	////////// Generating the heightmap

	const GLint w = dest -> w, h = dest -> h;

	WITH_SURFACE_PIXEL_ACCESS(src,
		WITH_SURFACE_PIXEL_ACCESS(dest,

			for (GLint y = 0; y < h; y++) {
				sdl_pixel_component_t* const dest_pixel = read_surface_pixel(dest, 0, y);

				for (GLint x = 0; x < w; x++) {
					const sdl_pixel_t pixel = *(sdl_pixel_t*) read_surface_pixel(src, x, y);

					sdl_pixel_component_t r, g, b;
					SDL_GetRGB(pixel, src -> format, &r, &g, &b);

					const sdl_pixel_component_t height = (r + g + b) / 3;
					const GLfloat upscaled_height = glm_min(height * heightmap_scale, constants.max_byte_value);
					dest_pixel[x] = (sdl_pixel_component_t) upscaled_height;
				}
			}
		);
	);
}

////////// This code concerns normal map creation.

static GLint int_min(const GLint val, const GLint lower) {
	return (val < lower) ? val : lower;
}

static GLint int_max(const GLint val, const GLint upper) {
	return (val > upper) ? val : upper;
}

static GLint limit_int_to_domain(const GLint val, const GLint lower, const GLint upper) {
	return int_min(int_max(val, lower), upper);
}

static sdl_pixel_component_t sobel_sample(const SDL_Surface* const surface, const GLint x, const GLint y) {
	return *(sdl_pixel_component_t*) read_surface_pixel(surface, x, y);
}

/* This function is based on these sources:
- https://en.wikipedia.org/wiki/Sobel_operator
- https://www.shadertoy.com/view/Xtd3DS

It's assumed that `src` has the same size as `dest`. */
static void generate_normal_map(SDL_Surface* const src, SDL_Surface* const dest, const GLint subtexture_h) {
	const GLint w = dest -> w, h = dest -> h;
	const SDL_PixelFormat* const format = dest -> format;

	const GLfloat half_max_byte_value = 0.5f * constants.max_byte_value;

	WITH_SURFACE_PIXEL_ACCESS(src,
		WITH_SURFACE_PIXEL_ACCESS(dest,

			for (GLint y = 0; y < h; y++) {
				sdl_pixel_t* const dest_pixel = read_surface_pixel(dest, 0, y);

				const GLint subtexture_top = (y / subtexture_h) * subtexture_h;
				const GLint subtexture_bottom = subtexture_top + subtexture_h - 1;

				for (GLint x = 0; x < w; x++) {
					const GLint
						left_x = int_max(x - 1, 0),             right_x = int_min(x + 1, w - 1),
						top_y = int_max(y - 1, subtexture_top), bottom_y = int_min(y + 1, subtexture_bottom);

					const sdl_pixel_component_t // These samples are in a range from 0 to `constants.max_byte_value`
						tl = sobel_sample(src, left_x, top_y),  tm = sobel_sample(src, x, top_y),
						tr = sobel_sample(src, right_x, top_y), ml = sobel_sample(src, left_x, y),
						mr = sobel_sample(src, right_x, y),     bl = sobel_sample(src, left_x, bottom_y),
						bm = sobel_sample(src, x, bottom_y),    br = sobel_sample(src, right_x, bottom_y);

					/* The x and y components of this are the result of the Sobel operator.
					Byte overflow will not happen with the components, since they are promoted to ints. */

					vec3 normal = {
						(-bl - (ml << 1) - tl) + (tr + (mr << 1) + br),
						(-tr - (tm << 1) - tl) + (bl + (bm << 1) + br)
					};

					const GLfloat // These are in a range of 0 to 1
						gx = normal[0] * constants.one_over_max_byte_value,
						gy = normal[1] * constants.one_over_max_byte_value;

					normal[2] = sqrtf(fabsf(1.0f - (gx * gx + gy * gy))) * constants.max_byte_value;

					glm_vec3_normalize(normal);
					glm_vec3_scale(normal, half_max_byte_value, normal);
					glm_vec3_adds(normal, half_max_byte_value, normal);

					dest_pixel[x] = SDL_MapRGBA(format,
						(sdl_pixel_component_t) normal[0],
						(sdl_pixel_component_t) normal[1],
						(sdl_pixel_component_t) normal[2],

						// Inverse height value for parallax mapping
						constants.max_byte_value - sobel_sample(src, x, y)
					);
				}
			}
		);
	);
}

////////// This code concerns Gaussian blur (the normal map input is blurred to cut out high frequencies from the Sobel operator).

static GLfloat* compute_1D_gaussian_kernel(const signed_byte radius, const GLfloat std_dev) {
	const signed_byte kernel_length = radius * 2 + 1;

	GLfloat* const kernel = alloc((size_t) kernel_length, sizeof(GLfloat)), sum = 0.0f;
	const GLfloat one_over_two_times_std_dev_squared = 1.0f / (2.0f * std_dev * std_dev);

	for (signed_byte x = 0; x < kernel_length; x++) {
		const signed_byte dx = x - radius;
		const GLfloat weight = expf(-(dx * dx) * one_over_two_times_std_dev_squared);
		kernel[x] = weight;
		sum += weight;
	}

	const GLfloat one_over_sum = 1.0f / sum;
	for (signed_byte i = 0; i < kernel_length; i++) kernel[i] *= one_over_sum;

	return kernel;
}

// It's assumed that `src` has the same size as `dest`, and that they both have the same format.
static void do_separable_gaussian_blur_pass(
	SDL_Surface* const src, SDL_Surface* const dest,
	const GLfloat* const kernel, const GLint subtexture_h,
	const signed_byte kernel_radius, const bool blur_is_vertical) {

	const GLint w = dest -> w, h = dest -> h;

	WITH_SURFACE_PIXEL_ACCESS(src,
		WITH_SURFACE_PIXEL_ACCESS(dest,

			for (GLint y = 0; y < h; y++) {
				sdl_pixel_component_t* const dest_pixel = read_surface_pixel(dest, 0, y);

				const GLint subtexture_top = (y / subtexture_h) * subtexture_h;
				const GLint subtexture_bottom = subtexture_top + subtexture_h - 1;

				for (GLint x = 0; x < w; x++) {
					GLuint blurred_pixel = 0.0f;

					for (signed_byte i = -kernel_radius; i <= kernel_radius; i++) {
						GLint fx = x, fy = y; // `f` = filter
						if (blur_is_vertical) fy += i; else fx += i;

						const sdl_pixel_component_t src_pixel = *(sdl_pixel_component_t*)
							read_surface_pixel(src,
								limit_int_to_domain(fx, 0, w - 1),
								limit_int_to_domain(fy, subtexture_top, subtexture_bottom)
							);

						blurred_pixel += (sdl_pixel_component_t) (src_pixel * kernel[i + kernel_radius]);
					}

					dest_pixel[x] = (sdl_pixel_component_t) blurred_pixel;
				}
			}
		);
	);
}

static void get_texture_metadata(
	const TextureType type,
	GLint* const subtexture_w, GLint* const subtexture_h,
	GLint* const num_subtextures, GLint* const wrap_mode,
	GLint mag_min_filter[2]) {

	const GLint level = 0;

	glGetTexLevelParameteriv(type, level, GL_TEXTURE_WIDTH, subtexture_w);
	glGetTexLevelParameteriv(type, level, GL_TEXTURE_HEIGHT, subtexture_h);

	if (type == TexSet) glGetTexLevelParameteriv(type, level, GL_TEXTURE_DEPTH, num_subtextures);
	else *num_subtextures = 1;

	// The wrap mode for each axis is the same, so only for 'S' (or across) is fine
	glGetTexParameteriv(type, GL_TEXTURE_WRAP_S, wrap_mode);
	glGetTexParameteriv(type, GL_TEXTURE_MAG_FILTER, mag_min_filter);
	glGetTexParameteriv(type, GL_TEXTURE_MIN_FILTER, mag_min_filter + 1);
}

// Note: level init is almost instant when this just returns 0; so GPU parallelization could be great here
GLuint init_normal_map_from_albedo_texture(const GLuint albedo_texture,
	const TextureType type, const NormalMapConfig* const config) {

	/* How this function works:

	- First, query OpenGL about information about the texture set, like its dimensions, and its filters used.
	- Then, define two grayscale surfaces, #1 and #2, and one RGBA surface.
	- Copy the surface in from disk into the RGBA surface.
	- Make a heightmap of the RGBA surface to #1.
	- Blur #1 horizontally to #2.
	- Blur #2 vertically to #1.
	- Generate a normal map of #1 to the RGBA surface.
	- Upload the RGBA surface to the GPU as a texture set of normal maps.

	Note: normal maps are not interleaved with the texture set because if gamma correction is used,
	the texture set will be in SRGB, and normal maps should be in a linear color space. */

	if (type != TexPlain && type != TexSet)
		FAIL(CreateTexture, "%s", "Normal map creation failed: unsupported texture type");


	////////// Querying OpenGL for information about the texture set

	GLint subtexture_w, subtexture_h, num_subtextures, wrap_mode, mag_min_filter[2];

	use_texture(type, albedo_texture);
	get_texture_metadata(type, &subtexture_w, &subtexture_h, &num_subtextures, &wrap_mode, mag_min_filter);

	////////// Uploading the texture to the CPU

	const GLfloat rescale_factor = config -> rescale_factor;
	const bool rescaling = rescale_factor != 1.0f;
	const GLint unscaled_subtexture_w = subtexture_w, unscaled_subtexture_h = subtexture_h, level = 0;

	if (rescaling) {
		subtexture_w = (GLint) (subtexture_w * rescale_factor);
		subtexture_h = (GLint) (subtexture_h * rescale_factor);
	}

	const GLint cpu_buffers_h = subtexture_h * num_subtextures;

	SDL_Surface
		*const rgba_surface = init_blank_surface(subtexture_w, cpu_buffers_h),
		*const grayscale_buffer_1 = init_blank_grayscale_surface(subtexture_w, cpu_buffers_h),
		*const grayscale_buffer_2 = init_blank_grayscale_surface(subtexture_w, cpu_buffers_h);

	//////////

	if (rescaling) { // If rescaling, copying the texture on the GPU into a surface with the same size, and then upscaling that surface to `rgba_surface`
		SDL_Surface* const surface_with_src_size = init_blank_surface(
			unscaled_subtexture_w, unscaled_subtexture_h * num_subtextures);

		WITH_SURFACE_PIXEL_ACCESS(surface_with_src_size,
			glGetTexImage(type, level, OPENGL_INPUT_PIXEL_FORMAT,
				OPENGL_COLOR_CHANNEL_TYPE, surface_with_src_size -> pixels);
		);

		SDL_BlitScaled(surface_with_src_size, NULL, rgba_surface, NULL);
		deinit_surface(surface_with_src_size);
	}
	else { // Otherwise, copy the texture directly to `rgba_surface`
		WITH_SURFACE_PIXEL_ACCESS(rgba_surface,
			glGetTexImage(type, level, OPENGL_INPUT_PIXEL_FORMAT,
				OPENGL_COLOR_CHANNEL_TYPE, rgba_surface -> pixels);
		);
	}

	////////// Making a heightmap

	generate_heightmap(rgba_surface, grayscale_buffer_1, config -> heightmap_scale);

	////////// Blurring it (if needed), and then making a normal map

	const signed_byte blur_radius = config -> blur_radius;
	const GLfloat blur_std_dev = config -> blur_std_dev;

	if (blur_radius != 0 && blur_std_dev != 0.0f) {
		GLfloat* const blur_kernel = compute_1D_gaussian_kernel(blur_radius, blur_std_dev);

		do_separable_gaussian_blur_pass( // Blurring #1 to #2 horizontally
			grayscale_buffer_1, grayscale_buffer_2, blur_kernel, subtexture_h, blur_radius, false);

		do_separable_gaussian_blur_pass( // Blurring #2 to #1 vertically
			grayscale_buffer_2, grayscale_buffer_1, blur_kernel, subtexture_h, blur_radius, true);

		dealloc(blur_kernel);
	}

	// Making a normal map of #1 to `rgba_surface`
	generate_normal_map(grayscale_buffer_1, rgba_surface, subtexture_h);

	////////// Putting the normal map in GPU memory

	const TextureFilterMode min_filter = (TextureFilterMode) mag_min_filter[1];
	const GLuint normal_map_set = preinit_texture(type, (TextureWrapMode) wrap_mode,
		(TextureFilterMode) mag_min_filter[0], min_filter, config -> use_anisotropic_filtering);

	WITH_SURFACE_PIXEL_ACCESS(rgba_surface, // Copying `rgba_surface` to a new texture on the GPU
		init_texture_data(type, (GLsizei[]) {subtexture_w, subtexture_h, num_subtextures}, OPENGL_INPUT_PIXEL_FORMAT,
			OPENGL_NORMAL_MAP_INTERNAL_PIXEL_FORMAT, OPENGL_COLOR_CHANNEL_TYPE, rgba_surface -> pixels);
	);

	if (min_filter == TexLinearMipmapped || min_filter == TexTrilinear) init_texture_mipmap(type);

	////////// Deinitialization

	deinit_surface(grayscale_buffer_1);
	deinit_surface(grayscale_buffer_2);
	deinit_surface(rgba_surface);

	return normal_map_set;
}
