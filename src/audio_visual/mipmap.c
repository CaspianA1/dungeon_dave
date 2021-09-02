SDL_Rect get_mipmap_crop(const ivec size, const byte depth_offset) {
	const int orig_size = size.x * 2 / 3; // TODO: don't recalculate orig_size
	// and TODO: size to an int

	SDL_Rect dest = {
		.x = (depth_offset == 0) ? 0 : orig_size,
		.y = 0,
		.w = orig_size >> depth_offset,
		.h = size.y >> depth_offset
	};

	for (byte i = 2; i < depth_offset + 1; i++)
		dest.y += size.y >> (i - 1);

	return dest;
}

inlinable int closest_pow_2(const int x) {
	return 1 << (sizeof(x) * 8 - num_leading_zeroes(x));
}

SDL_Rect get_mipmap_crop_from_wall(const Sprite* const sprite, const int wall_h) {
	/* A texture is displayed best if each texture pixel corresponds to one on-screen pixel,
	meaning that for a 64x64 texture, it looks best if there is a 1:1 mapping for 64x64 screen units.
	This function finds the nearest exponent for a power of 2 of the texture and turns that into a depth offset.

	Example: a 64x64 texture with a projected wall height of 27 will look best if its mipmap level's height is 32,
	as that is the closest to a 1:1 mapping. Since the nearest power of 2 is 32, the exponent is 5 (2 ** 5 == 32),
	meaning that the depth offset should be the max power of two for the full-size texture minus the current
	nearest power of two. In this case, that is 6 - 1, which means it should have a depth offset of 1. */

	const ivec size = sprite -> size;
	const int orig_size = size.x * 2 / 3; // orig_size = full width and height of the full-resolution texture

	int closest_exp_2 = closest_pow_2(wall_h);
	if (closest_exp_2 > orig_size) closest_exp_2 = orig_size; // assumes that orig_size is a power of 2

	const byte depth_offset = exp_for_pow_of_2(orig_size) - exp_for_pow_of_2(closest_exp_2);
	return get_mipmap_crop(size, depth_offset);
}

SDL_Surface* load_mipmap(SDL_Surface* const surface, byte* const depth) {
	SDL_Surface* const mipmap = SDL_CreateRGBSurfaceWithFormat(0,
		surface -> w + (surface -> w >> 1), surface -> h, 32, PIXEL_FORMAT);

	SDL_Rect dest = {.w = surface -> w, .h = surface -> h};
	while (dest.w != 0 || dest.h != 0) {
		if (*depth == 0) dest.x = 0;
		else dest.x = surface -> w;

		if (*depth <= 1) dest.y = 0;
		else dest.y += surface -> h >> (*depth - 1);

		SDL_BlitScaled(surface, NULL, mipmap, &dest);
		box_blur_image_portion(mipmap, dest, *depth / 3);

		dest.w >>= 1;
		dest.h >>= 1;
		(*depth)++;
	}

	/*
	system("mkdir imgs");
	static byte id = 0;
	char buf[15];
	sprintf(buf, "imgs/out_%d.bmp", id);
	SDL_SaveBMP(mipmap, buf);
	id++;
	*/

	return mipmap;
}
