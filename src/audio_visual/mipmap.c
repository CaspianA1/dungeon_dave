Uint32* read_surface_pixel(const SDL_Surface* const surface, const int x, const int y, const int bpp) {
	return (Uint32*) ((Uint8*) surface -> pixels + y * surface -> pitch + x * bpp);
}

#ifdef ANTIALIASED_MIPMAPPING

void antialiased_downscale_by_2(const SDL_Surface* const orig,
	SDL_Surface* const mipmap, const ivec dest_origin, const byte scale_factor) {

	const byte dec_scale_factor = scale_factor - 1;
	const int orig_size = orig -> w;

	const SDL_PixelFormat* const format = orig -> format;
	const int bpp = format -> BytesPerPixel, inc_across = 1 << dec_scale_factor;

	for (int y = 0; y < orig_size; y += inc_across) {
		for (int x = 0; x < orig_size; x += inc_across) {
			const int dec_x = x - 1, dec_y = y - 1, inc_x = x + 1, inc_y = y + 1;

			const ivec pixel_group[9] = {
				{dec_x, dec_y}, {x, dec_y}, {inc_x, dec_y},
				{dec_x, y}, 	{x, y}, 	{inc_x, y},
				{dec_x, inc_y}, {x, inc_y}, {inc_x, inc_y}
			};

			int sum[4] = {0, 0, 0, 0};
			byte valid_neighbor_sum = 0;

			for (byte i = 0; i < 9; i++) {
				const ivec pixel_pos = pixel_group[i];

				if (pixel_pos.x >= 0 && pixel_pos.y >= 0 && pixel_pos.x < orig_size && pixel_pos.y < orig_size) {
					const Uint32 neighbor = *read_surface_pixel(orig, pixel_pos.x, pixel_pos.y, bpp);

					sum[0] += (byte) (neighbor >> 24);
					sum[1] += (byte) (neighbor >> 16);
					sum[2] += (byte) (neighbor >> 8);
					sum[3] += (byte) neighbor;
					valid_neighbor_sum++;
				}
			}

			const Uint32 result = SDL_MapRGBA(format, sum[1] / valid_neighbor_sum, sum[2] / valid_neighbor_sum,
				sum[3] / valid_neighbor_sum, sum[0] / valid_neighbor_sum);

			*read_surface_pixel(mipmap, (x >> dec_scale_factor) + dest_origin.x, (y >> dec_scale_factor) + dest_origin.y, bpp) = result;
		}
	}
}

#endif

//////////

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

inlinable int nearest_pow_2(const int num) {
	int next_pow = num - 1;

	next_pow |= next_pow >> 1;
	next_pow |= next_pow >> 2;
	next_pow |= next_pow >> 4;
	next_pow |= next_pow >> 8;
	next_pow |= next_pow >> 16;
	next_pow++;

	const int prev_pow = next_pow >> 1;

	// smallest difference between powers of 2
	return (next_pow - num) > (num - prev_pow) ? prev_pow : next_pow;
}

#define one_plus_exp_for_pow_2 __builtin_ffs

SDL_Rect get_mipmap_crop_from_wall(const Sprite* const mipmap, const int wall_h) {
	/* A texture is displayed best if each texture pixel corresponds to one on-screen pixel,
	meaning that for a 64x64 texture, it looks best if there is a 1:1 mapping for 64x64 screen units.
	This function finds the nearest exponent for a power of 2 of the texture and turns that into a depth offset.

	Example: a 64x64 texture with a projected wall height of 27 will look best if its mipmap level's height is 32,
	as that is the closest to a 1:1 mapping. Since the nearest power of 2 is 32, the exponent is 5 (2 ** 5 == 32),
	meaning that the depth offset should be the max power of two for the full-size texture minus the current
	nearest power of two. In this case, that is 6 - 5 (b/c 2 ** 6 == 64), meaning the depth offset is 1). */

	const int orig_size = mipmap -> size.y; // orig_size = full w and h of the original full-res texture

	int pow_2 = nearest_pow_2(wall_h);
	if (pow_2 > orig_size) pow_2 = orig_size;

	const byte depth_offset = one_plus_exp_for_pow_2(orig_size) - one_plus_exp_for_pow_2(pow_2);
	return get_mipmap_crop(orig_size, depth_offset);
}

inlinable byte is_pow_of_2(const int num) {
	return (num & (num - 1)) == 0;
}

// returns null if the image dimensions aren't powers of 2
SDL_Surface* load_mipmap(SDL_Surface* const image, byte* const depth_ref) {
	const int image_size = image -> w; // image must have uniform dimensions
	if (!is_pow_of_2(image_size) || !is_pow_of_2(image -> h)) return NULL;

	SDL_Surface* const mipmap = SDL_CreateRGBSurfaceWithFormat(0,
		image_size + (image_size >> 1), image_size, PIXEL_FORMAT_DEPTH, PIXEL_FORMAT);

	byte depth = 0;

	#ifdef ANTIALIASED_MIPMAPPING

	SDL_LockSurface(image);
	SDL_LockSurface(mipmap);

	int dest_size = image_size;
	ivec dest_origin = {0, 0};
	while (dest_size != 0) {
		if (depth >= 2) dest_origin.y += image_size >> (depth - 1);
		antialiased_downscale_by_2(image, mipmap, dest_origin, depth + 1);

		dest_origin.x = image_size;
		dest_size >>= 1;
		depth++;
	}

	SDL_UnlockSurface(image);
	SDL_UnlockSurface(mipmap);

	#else

	SDL_Rect dest = {0, 0, image_size, image_size};
	SDL_Rect last_dest = dest;
	SDL_BlitSurface(image, NULL, mipmap, &dest);

	while (dest.w != 0) {
		if (depth >= 2) dest.y += image_size >> (depth - 1);

		SDL_BlitScaled(mipmap, &last_dest, mipmap, &dest);
		last_dest = dest;

		dest.x = image_size;
		dest.w >>= 1;
		dest.h >>= 1;
		depth++;
	}

	#endif

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

	*depth_ref = depth;
	return mipmap;
}
