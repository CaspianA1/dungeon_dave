inlinable byte is_pow_of_2(const int num) {
	return (num & (num - 1)) == 0;
}

Sprite init_sprite(const char* const path, const byte enable_mipmap) {
	SDL_Surface* surface = SDL_LoadBMP(path);

	if (surface == NULL) FAIL("Could not load a surface with the path of %s\n", path);

	SDL_Surface* const converted_surface = SDL_ConvertSurfaceFormat(surface, PIXEL_FORMAT, 0);
	SDL_FreeSurface(surface);
	surface = converted_surface;

	Sprite sprite = {.max_mipmap_depth = 0};

	if (enable_mipmap) {
		SDL_Surface* const mipmap = load_mipmap(surface, &sprite.max_mipmap_depth);
		SDL_FreeSurface(surface);
		surface = mipmap;
	}

	sprite.size = (ivec) {surface -> w, surface -> h};

	if (enable_mipmap && (!is_pow_of_2(sprite.size.x * 2 / 3) || !is_pow_of_2(sprite.size.y))) // mipmaps need powers of 2 dimensions
		FAIL("The sprite with the path %s must have dimensions that are powers of two!\n", path);

	sprite.texture = SDL_CreateTextureFromSurface(screen.renderer, surface);

	SDL_FreeSurface(surface);
	return sprite;
}

PSprite init_psprite(const char* const path) {
	SDL_Surface* const unconverted_surface = SDL_LoadBMP(path);
	if (unconverted_surface == NULL) FAIL("Could not load a surface with the path of %s\n", path);
	SDL_Surface* const surface = SDL_ConvertSurfaceFormat(unconverted_surface, PIXEL_FORMAT, 0);
	if (surface == NULL) FAIL("Could not convert a surface type for path %s\n", path);

	SDL_FreeSurface(unconverted_surface);
	SDL_LockSurface(surface);

	const ivec size = {surface -> w, surface -> h};

	PSprite p = {
		.texture = SDL_CreateTexture(screen.renderer, PIXEL_FORMAT, SDL_TEXTUREACCESS_STREAMING, size.x, size.y),
		.size = size.x
	};

	SDL_LockTexture(p.texture, NULL, &p.pixels, &p.pitch);
	memcpy(p.pixels, surface -> pixels, p.pitch * size.y);
	SDL_UnlockTexture(p.texture);

	SDL_UnlockSurface(surface);
	SDL_FreeSurface(surface);

	return p;
}
