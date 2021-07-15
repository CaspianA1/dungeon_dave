const byte mipmap_depth = 10;

typedef struct {
	const int orig_w;
	SDL_Surface* const surface;
} Mipmap;

SDL_Rect get_mipmap_crop(const Mipmap mipmap, const byte depth_level) {
	SDL_Rect dest = {
		.x = (depth_level == 0) ? 0 : mipmap.orig_w,
		.y = 0,
		.w = mipmap.orig_w >> depth_level,
		.h = mipmap.surface -> h >> depth_level
	};

	for (byte i = 2; i < depth_level + 1; i++)
		dest.y += mipmap.surface -> h >> (i - 1);

	return dest;
}

SDL_Rect get_mipmap_crop_from_dist(const Mipmap mipmap, const double dist, const double max_dist) {
	double ratio = dist / max_dist;
	return get_mipmap_crop(mipmap, (ratio > 1.0) ? 1.0 : ratio);
}

void make_mipmap(SDL_Surface* const surface) {
	Uint32 rmask, gmask, bmask, amask;

	#if SDL_BYTEORDER == SDL_BIG_ENDIAN
		rmask = 0xFF000000, gmask = 0x00FF0000, bmask = 0x0000FF00, amask = 0x000000FF;
	#else
		rmask = 0x000000FF, gmask = 0x0000FF00, bmask = 0x00FF0000, amask = 0xFF000000;
	#endif

	Mipmap mipmap = {
		surface -> w,
		SDL_CreateRGBSurface(0, surface -> w + surface -> w / 2, surface -> h, 32, rmask, gmask, bmask, amask)
	};

	/*
	for (byte i = 0; i < mipmap_depth; i++) {
		SDL_Rect dest = get_mipmap_crop(mipmap, i);
		DEBUG_RECT(dest);
		SDL_BlitScaled(surface, NULL, mipmap.surface, &dest);
	}
	*/

	SDL_Rect dest = {.w = surface -> w, .h = surface -> h};
	for (byte i = 0; i < mipmap_depth; i++) {
		if (i == 0) dest.x = 0;
		else dest.x = surface -> w;

		if (i <= 1) dest.y = 0;
		else dest.y += surface -> h >> (i - 1);

		// DEBUG_RECT(dest);
		SDL_BlitScaled(surface, NULL, mipmap.surface, &dest);

		dest.w >>= 1;
		dest.h >>= 1;
	}

	if (SDL_SaveBMP(mipmap.surface, "test_mipmap.bmp") < 0)
		printf("Error saving a bitmap: %s\n", SDL_GetError());
	else printf("Succeeded in saving a bitmap\n");
}

void mipmap_test(void) {
	SDL_Surface* const surface = SDL_LoadBMP("../assets/walls/dune.bmp");
	make_mipmap(surface);
}
