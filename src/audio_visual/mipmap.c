#ifdef OPENGL_TEXTURE_FILTERING

void create_filtered_texture(SDL_Surface* const surface, Sprite* const sprite, const DrawableType drawable_type) {
	sprite -> texture = SDL_CreateTextureFromSurface(screen.renderer, surface);

	SDL_GL_BindTexture(sprite -> texture, NULL, NULL);

	GLenum min_filter, mag_filter;
	byte generate_mipmap = 0;

	switch (drawable_type) {
		case D_Wall: case D_Thing:
			min_filter = GL_LINEAR_MIPMAP_LINEAR;
			mag_filter = GL_NEAREST;
			generate_mipmap = 1;
			break;
		case D_Overlay:
			min_filter = GL_LINEAR;
			mag_filter = GL_NEAREST;
			break;
		case D_Skybox:
			min_filter = GL_LINEAR;
			mag_filter = GL_LINEAR;
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);
	if (generate_mipmap) glGenerateMipmap(GL_TEXTURE_2D);

	SDL_GL_UnbindTexture(sprite -> texture);
}

#else

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

// Returns null if the image dimensions aren't powers of 2
SDL_Surface* load_mipmap(SDL_Surface* const image, byte* const depth_ref) {
	const int image_size = image -> w; // Image must have uniform dimensions
	if (!is_pow_of_2(image_size) || !is_pow_of_2(image -> h)) return NULL;

	SDL_Surface* const mipmap = SDL_CreateRGBSurfaceWithFormat(0,
		image_size + (image_size >> 1), image_size, PIXEL_FORMAT_DEPTH, PIXEL_FORMAT);

	byte depth = 0;

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

#endif
