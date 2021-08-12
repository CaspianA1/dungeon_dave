const double mipmap_depth_heuristic = 18.0;

inlinable SDL_Rect get_mipmap_crop(const ivec size, const byte depth_offset) {
	const int orig_w = size.x * 2 / 3;

	SDL_Rect dest = {
		.x = (depth_offset == 0) ? 0 : orig_w,
		.y = 0,
		.w = orig_w >> depth_offset,
		.h = size.y >> depth_offset
	};

	for (byte i = 2; i < depth_offset + 1; i++)
		dest.y += size.y >> (i - 1);

	return dest;
}

inlinable int closest_pow_2(const int x) {
	return 1 << (sizeof(x) * 8 - num_leading_zeroes(x));
}

inlinable SDL_Rect get_mipmap_crop_from_wall(const Sprite* const sprite, const int wall_h) {
	/* A texture is displayed best if each texture pixel corresponds to one on-screen pixel,
	meaning that for a 64x64 texture, it looks best if there is a 1:1 mapping for 64x64 screen units.
	This function finds the nearest exponent for a power of 2 of the texture and turns that into a depth offset.

	Example: a 64x64 texture with a projected wall height of 27 will look best if its mipmap level's height is 32,
	as that is the closest to a 1:1 mapping. Since the nearest power of 2 is 32, the exponent is 5 (2 ** 5 == 32),
	meaning that the depth offset should be the max power of two for the full-size texture minus the current
	nearest power of two. In this case, that is 6 - 1, which means it should have a depth offset of 1. */

	const ivec size = sprite -> size;
	const int max_size = size.x * 2 / 3; // max size = full size of the full-resolution texture

	int closest_exp_2 = closest_pow_2(wall_h);
	if (closest_exp_2 > max_size) closest_exp_2 = max_size; // assumes that max_size is a power of 2

	const byte depth_offset = exp_for_pow_of_2(max_size) - exp_for_pow_of_2(closest_exp_2);
	return get_mipmap_crop(size, depth_offset);
}

SDL_Surface* load_mipmap(SDL_Surface* const surface, byte* const depth) {
	SDL_Surface* const mipmap = SDL_CreateRGBSurfaceWithFormat(0,
		surface -> w + (surface -> w >> 1), surface -> h, 32, PIXEL_FORMAT);

	void blur_image_portion(SDL_Surface* const, SDL_Rect, const int);

	SDL_Rect dest = {.w = surface -> w, .h = surface -> h};
	while (dest.w != 0 || dest.h != 0) {
		if (*depth == 0) dest.x = 0;
		else dest.x = surface -> w;

		if (*depth <= 1) dest.y = 0;
		else dest.y += surface -> h >> (*depth - 1);

		SDL_BlitScaled(surface, NULL, mipmap, &dest);
		blur_image_portion(mipmap, dest, *depth);

		dest.w >>= 1;
		dest.h >>= 1;
		(*depth)++;
	}

	/*
	static byte id = 0;
	char buf[15];
	sprintf(buf, "imgs/out_%d.bmp", id);
	SDL_SaveBMP(mipmap, buf);
	id++;
	*/

	return mipmap;
}

//////////

inlinable Uint32* read_surface_pixel(const SDL_Surface* const surface, const int x, const int y, const int bpp) {
	return (Uint32*) ((Uint8*) surface -> pixels + y * surface -> pitch + x * bpp);
}

void blur_image_portion(SDL_Surface* const image, SDL_Rect crop, const int blur_size) { // box blur
	typedef __v4si color4;

	const int src_w = image -> w, src_h = image -> h;
	SDL_Surface* const blurred_crop = SDL_CreateRGBSurfaceWithFormat(0, crop.w, crop.h, 32, PIXEL_FORMAT);
	SDL_LockSurface(blurred_crop);
	SDL_LockSurface(image);

	const SDL_PixelFormat* const format = image -> format;
	const int bpp = format -> BytesPerPixel;
 	for (int y = crop.y; y < crop.y + crop.h; y++) {
		for (int x = crop.x; x < crop.x + crop.w; x++) {
			color4 sum = {0, 0, 0, 0};
			int blur_sum_factor = 0;
			for (int py = -blur_size; py <= blur_size; py++) {
				for (int px = -blur_size; px <= blur_size; px++) {
					const int x1 = x + px, y1 = y + py;
					if (x1 < 0 || y1 < 0) continue;
					else if (x1 >= src_w || y1 >= src_h) break;

					const Uint32 pixel = *read_surface_pixel(image, x1, y1, bpp);

					byte r, g, b, a;
					SDL_GetRGBA(pixel, format, &r, &g, &b, &a);
					sum += (color4) {r, g, b, a};
	
					blur_sum_factor++;
                }
			}

			// TODO: figure out why the edges are close to black

			if (blur_sum_factor == 0) blur_sum_factor = 1;
			const byte out[4] = {
				sum[0] / blur_sum_factor,
				sum[1] / blur_sum_factor,
				sum[2] / blur_sum_factor,
				sum[3] / blur_sum_factor
			};

			const Uint32 blurred_pixel = SDL_MapRGBA(format, out[0], out[1], out[2], out[3]);
			*read_surface_pixel(blurred_crop, x - crop.x, y - crop.y, bpp) = blurred_pixel;
		}
	}

	SDL_UnlockSurface(blurred_crop);
	SDL_UnlockSurface(image);

	SDL_BlitSurface(blurred_crop, NULL, image, &crop);
	SDL_FreeSurface(blurred_crop);
}

void blur_test(void) {
	SDL_Surface* const unconverted_image = SDL_LoadBMP("assets/walls/smooth_viney_bricks.bmp");
	SDL_Surface* const image = SDL_ConvertSurfaceFormat(unconverted_image, PIXEL_FORMAT, 0);
	SDL_FreeSurface(unconverted_image);

	blur_image_portion(image, (SDL_Rect) {0, 0, image -> w, image -> h}, 2);
	SDL_SaveBMP(image, "out.bmp");
	SDL_FreeSurface(image);

	exit(0);
}
