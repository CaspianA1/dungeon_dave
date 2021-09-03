void antialias_test(void) {
	SDL_Surface* const unconverted_image = SDL_LoadBMP("assets/walls/horses.bmp");
	SDL_Surface* const image = SDL_ConvertSurfaceFormat(unconverted_image, PIXEL_FORMAT, 0);
	SDL_FreeSurface(unconverted_image);

	const byte scale = 3;
	const byte dec_scale = scale - 1;

	SDL_Surface* const unfiltered = SDL_CreateRGBSurfaceWithFormat(0, image -> w >> dec_scale, image -> h >> dec_scale, 32, PIXEL_FORMAT);
	SDL_BlitScaled(image, NULL, unfiltered, NULL);
	SDL_SaveBMP(unfiltered, "unfiltered.bmp");
	SDL_FreeSurface(unfiltered);

	SDL_Surface* antialiased_downscale_by_2(const SDL_Surface* const, const byte);
	SDL_Surface* const filtered = antialiased_downscale_by_2(image, scale);
	SDL_SaveBMP(filtered, "filtered.bmp");
	SDL_FreeSurface(filtered);

	exit(0);
}

SDL_Surface* antialiased_downscale_by_2(const SDL_Surface* const orig, const byte scale_factor) {
	const byte dec_scale_factor = scale_factor - 1;
	const int orig_size = orig -> w;
	const int downscaled_size = orig_size >> dec_scale_factor;

	SDL_Surface* const filtered = SDL_CreateRGBSurfaceWithFormat(0, downscaled_size, downscaled_size, 32, PIXEL_FORMAT);
	const SDL_PixelFormat* const format = filtered -> format;
	const int bpp = format -> BytesPerPixel, inc_across = 1 << dec_scale_factor;

	for (int y = 0; y < orig_size; y += inc_across) {
		for (int x = 0; x < orig_size; x += inc_across) {
			const int dec_x = x - 1, dec_y = y - 1, inc_x = x + 1, inc_y = y + 1;
			const ivec pixel_group[9] = {
				{dec_x, dec_y}, {x, dec_y}, {inc_x, dec_y},
				{dec_x, y}, 	{x, y}, 	{inc_x, y},
				{dec_x, inc_y}, {x, inc_y}, {inc_x, inc_y}
			};

			color4_t sum = {0, 0, 0, 0};
			byte valid_neighbor_sum = 0;

			for (byte i = 0; i < 9; i++) {
				const ivec pixel_pos = pixel_group[i];

				if (pixel_pos.x >= 0 && pixel_pos.y >= 0 && pixel_pos.x < orig_size && pixel_pos.y < orig_size) {
					const Uint32 neighbor_val = *read_surface_pixel(orig, pixel_pos.x, pixel_pos.y, bpp);
					byte r, g, b, a;
					SDL_GetRGBA(neighbor_val, format, &r, &g, &b, &a);
					sum += (color4_t) {r, g, b, a};
					valid_neighbor_sum++;
				}
			}

			*read_surface_pixel(filtered, x >> dec_scale_factor, y >> dec_scale_factor, bpp) = SDL_MapRGBA(format,
				sum[0] / valid_neighbor_sum, sum[1] / valid_neighbor_sum,
				sum[2] / valid_neighbor_sum, sum[3] / valid_neighbor_sum);
		}
	}
	return filtered;
}

//////////

void filter_mipmap_level(SDL_Surface* const image, SDL_Surface* const mipmap, SDL_Rect* const dest, const byte depth) {
	SDL_Surface* mipmap_level;
	const byte create_downscaled = depth != 0;
	mipmap_level = create_downscaled ? antialiased_downscale_by_2(image, depth + 1) : image;

	SDL_BlitScaled(mipmap_level, NULL, mipmap, dest);
	if (create_downscaled) SDL_FreeSurface(mipmap_level);
}

inlinable SDL_Rect get_mipmap_crop(const int orig_size, const byte depth_offset) {
	const int crop_size = orig_size >> depth_offset;

	SDL_Rect dest = {
		.x = (depth_offset == 0) ? 0 : orig_size,
		.y = 0,
		.w = crop_size,
		.h = crop_size
	};

	for (byte i = 2; i < depth_offset + 1; i++)
		dest.y += orig_size >> (i - 1);

	return dest;
}

inlinable int closest_pow_2(const int x) {
	return 1 << (sizeof(x) * 8 - num_leading_zeroes(x));
}

inlinable SDL_Rect get_mipmap_crop_from_wall(const Sprite* const mipmap, const int wall_h) {
	/* A texture is displayed best if each texture pixel corresponds to one on-screen pixel,
	meaning that for a 64x64 texture, it looks best if there is a 1:1 mapping for 64x64 screen units.
	This function finds the nearest exponent for a power of 2 of the texture and turns that into a depth offset.

	Example: a 64x64 texture with a projected wall height of 27 will look best if its mipmap level's height is 32,
	as that is the closest to a 1:1 mapping. Since the nearest power of 2 is 32, the exponent is 5 (2 ** 5 == 32),
	meaning that the depth offset should be the max power of two for the full-size texture minus the current
	nearest power of two. In this case, that is 6 - 1, which means it should have a depth offset of 1. */

	const int orig_size = mipmap -> size.x * 2 / 3; // orig_size = full width and height of the original full-res texture

	int closest_exp_2 = closest_pow_2(wall_h);
	if (closest_exp_2 > orig_size) closest_exp_2 = orig_size; // assumes that orig_size is a power of 2

	const byte depth_offset = exp_for_pow_of_2(orig_size) - exp_for_pow_of_2(closest_exp_2);
	return get_mipmap_crop(orig_size, depth_offset);
}

SDL_Surface* load_mipmap(SDL_Surface* image, byte* const depth) {
	SDL_Surface* const mipmap = SDL_CreateRGBSurfaceWithFormat(0,
		image -> w + (image -> w >> 1), image -> h, 32, PIXEL_FORMAT);

	SDL_Rect dest = {.w = image -> w, .h = image -> h};
	while (dest.w != 0 || dest.h != 0) {
		if (*depth == 0) dest.x = 0;
		else dest.x = image -> w;

		if (*depth <= 1) dest.y = 0;
		else dest.y += image -> h >> (*depth - 1);

		filter_mipmap_level(image, mipmap, &dest, *depth);

		/*
		SDL_BlitScaled(image, NULL, mipmap, &dest);
		box_blur_image_portion(mipmap, dest, *depth / 3);
		*/

		dest.w >>= 1;
		dest.h >>= 1;
		(*depth)++;
	}

	/*
	static byte first = 1, id = 0;
	if (first) {
		system("mkdir -p imgs");
		first = 0;
	}

	char buf[15];
	sprintf(buf, "imgs/out_%d.bmp", id);
	SDL_SaveBMP(mipmap, buf);
	id++;
	*/

	return mipmap;
}
