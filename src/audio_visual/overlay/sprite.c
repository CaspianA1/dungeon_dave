SDL_Surface* load_surface(const char* const path) {
	SDL_Surface* surface = SDL_LoadBMP(path);
	if (surface == NULL) FAIL("Could not load a surface with the path of %s\n", path);

	SDL_Surface* const converted_surface = SDL_ConvertSurfaceFormat(surface, PIXEL_FORMAT, 0);
	if (converted_surface == NULL) FAIL("Could not convert a surface to the default pixel format: %s\n", path);
	SDL_FreeSurface(surface);
	return converted_surface;
}

Sprite init_sprite(const char* const path, const byte enable_mipmap) {
	SDL_Surface* surface = load_surface(path);

	Sprite sprite = {.max_mipmap_depth = 0};

	if (enable_mipmap) {
		SDL_Surface* const mipmap = load_mipmap(surface, &sprite.max_mipmap_depth);
		if (mipmap == NULL) {
			FAIL("The sprite with the path %s must have dimensions that are powers of two\n", path);
		}
		else {
			SDL_FreeSurface(surface); // free the previous surface
			surface = mipmap; // replace it with the mipmap
		}
	}

	sprite.size = (ivec) {surface -> w, surface -> h};
	sprite.texture = SDL_CreateTextureFromSurface(screen.renderer, surface); // make a texture from the surface
	SDL_FreeSurface(surface); // free the surface, it isn't used anymore

	return sprite;
}

PixSprite init_pix_sprite(const char* const path) {
	SDL_Surface* const surface = load_surface(path);
	SDL_LockSurface(surface);

	const int size = surface -> w;
	const int bytes = size * size * surface -> format -> BytesPerPixel;

	const PixSprite pix_sprite = {wmalloc(bytes), size};
	memcpy(pix_sprite.pixels, surface -> pixels, bytes);

	SDL_UnlockSurface(surface);
	SDL_FreeSurface(surface);

	return pix_sprite;
}
