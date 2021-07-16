const byte max_mipmap_depth = 5;
const double max_mipmap_dist = 40.0;

SDL_Rect get_mipmap_crop(const ivec size, const byte depth_offset) {
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

inlinable SDL_Rect get_mipmap_crop_from_dist(const ivec size, const double dist) {
	byte depth_offset = dist / max_mipmap_dist * max_mipmap_depth;
	if (depth_offset >= max_mipmap_depth) depth_offset = max_mipmap_depth - 1;
	return get_mipmap_crop(size, depth_offset);
}

SDL_Surface* load_mipmap(SDL_Surface* const surface) {
	const Uint32 color_masks[4] = { // extraction masks for red, green, blue, and alpha
		#if SDL_BYTEORDER == SDL_BIG_ENDIAN
		0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF
		#else
		0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000
		#endif
	};

	SDL_Surface* const mipmap = SDL_CreateRGBSurface(0, surface -> w + (surface -> w >> 1),
		surface -> h, 32, color_masks[0], color_masks[1], color_masks[2], color_masks[3]);

	SDL_Rect dest = {.w = surface -> w, .h = surface -> h};
	for (byte i = 0; i < max_mipmap_depth; i++) {
		if (i == 0) dest.x = 0;
		else dest.x = surface -> w;

		if (i <= 1) dest.y = 0;
		else dest.y += surface -> h >> (i - 1);

		SDL_BlitScaled(surface, NULL, mipmap, &dest);

		dest.w >>= 1;
		dest.h >>= 1;
	}

	return mipmap;
}
