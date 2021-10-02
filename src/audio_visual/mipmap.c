Uint32* read_surface_pixel(const SDL_Surface* const surface, const int x, const int y, const int bpp) {
	return (Uint32*) ((Uint8*) surface -> pixels + y * surface -> pitch + x * bpp);
}

void antialiased_downscale_by_2(const SDL_Surface* const orig,
	SDL_Surface* const mipmap, const ivec dest_corner, const byte scale_factor) {

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

			*read_surface_pixel(mipmap, (x >> dec_scale_factor) + dest_corner.x, (y >> dec_scale_factor) + dest_corner.y, bpp) = result;
		}
	}
}

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

int nearest_pow_2(int n) {
	int v = n; 

	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++; // next power of 2

	int x = v >> 1; // previous power of 2

	return (v - n) > (n - x) ? x : v;
}

SDL_Rect get_mipmap_crop_from_wall(const Sprite* const mipmap, const int wall_h) {
	/* A texture is displayed best if each texture pixel corresponds to one on-screen pixel,
	meaning that for a 64x64 texture, it looks best if there is a 1:1 mapping for 64x64 screen units.
	This function finds the nearest exponent for a power of 2 of the texture and turns that into a depth offset.

	Example: a 64x64 texture with a projected wall height of 27 will look best if its mipmap level's height is 32,
	as that is the closest to a 1:1 mapping. Since the nearest power of 2 is 32, the exponent is 5 (2 ** 5 == 32),
	meaning that the depth offset should be the max power of two for the full-size texture minus the current
	nearest power of two. In this case, that is 6 - 1, which means it should have a depth offset of 1. */

	const int orig_size = mipmap -> size.y; // orig_size = full w and h of the original full-res texture

	int closest_exp_2 = nearest_pow_2(wall_h);
	if (closest_exp_2 > orig_size) closest_exp_2 = orig_size; // assumes that orig_size is a power of 2
	const byte depth_offset = exp_for_pow_of_2(orig_size) - exp_for_pow_of_2(closest_exp_2);
	return get_mipmap_crop(orig_size, depth_offset);
}

SDL_Surface* load_mipmap(SDL_Surface* const image, byte* const depth) {
	SDL_Surface* const mipmap = SDL_CreateRGBSurfaceWithFormat(0,
		image -> w + (image -> w >> 1), image -> h, 32, PIXEL_FORMAT);

	#ifdef ANTIALIASED_MIPMAPPING
	SDL_LockSurface(image);
	SDL_LockSurface(mipmap);
	#else
	SDL_Rect top_left_corner = {0, 0, image -> w, image -> h};
	SDL_BlitSurface(image, NULL, mipmap, &top_left_corner);
	#endif


	SDL_Rect dest = {.w = image -> w, .h = image -> h};

	#ifndef ANTIALIASED_MIPMAPPING
	SDL_Rect last_dest = dest;
	last_dest.x = 0;
	last_dest.y = 0;
	#endif

	while (dest.w != 0 || dest.h != 0) {
		if (*depth == 0) dest.x = 0;
		else dest.x = image -> w;

		if (*depth <= 1) dest.y = 0;
		else dest.y += image -> h >> (*depth - 1);

		#ifdef ANTIALIASED_MIPMAPPING
		antialiased_downscale_by_2(image, mipmap, (ivec) {dest.x, dest.y}, *depth + 1);
		#else
		SDL_BlitScaled(mipmap, &last_dest, mipmap, &dest);
		last_dest = dest;
		#endif

		dest.w >>= 1;
		dest.h >>= 1;
		(*depth)++;
	}

	static byte first = 1, id = 0;
	if (first) {
		system("mkdir -p imgs");
		first = 0;
	}

	char buf[15];
	sprintf(buf, "imgs/out_%d.bmp", id);
	SDL_SaveBMP(mipmap, buf);
	id++;

	#ifdef ANTIALIASED_MIPMAPPING
	SDL_UnlockSurface(image);
	SDL_UnlockSurface(mipmap);
	#endif

	return mipmap;
}
