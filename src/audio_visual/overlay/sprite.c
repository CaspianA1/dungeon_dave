Sprite init_sprite(const char* const path) { // put in other file
	SDL_Surface* const surface = SDL_LoadBMP(path);
	if (surface == NULL) FAIL("Could not load a surface with the path of %s\n", path);

	const Sprite sprite = {SDL_CreateTextureFromSurface(screen.renderer, surface), {surface -> w, surface -> h}};
	SDL_FreeSurface(surface);
	return sprite;
}

inlinable Uint32* read_texture_row(const void* const pixels, const int pixel_pitch, const int y) {
	return (Uint32*) ((Uint8*) pixels + y * pixel_pitch);
}

PSprite init_psprite(const char* const path) {
	SDL_Surface* const unconverted_surface = SDL_LoadBMP(path);
	if (unconverted_surface == NULL) FAIL("Could not load a surface with the path of %s\n", path);
	SDL_Surface* const converted_surface = SDL_ConvertSurfaceFormat(unconverted_surface, PIXEL_FORMAT, 0);
	if (converted_surface == NULL) FAIL("Could not convert a surface type for path %s\n", path);
	SDL_FreeSurface(unconverted_surface);

	const ivec size = {converted_surface -> w, converted_surface -> h};

	PSprite p = {
		.texture = SDL_CreateTexture(screen.renderer, PIXEL_FORMAT,
			SDL_TEXTUREACCESS_STREAMING, size.x, size.y),
		.size = size.x
	};

	SDL_LockTexture(p.texture, NULL, &p.pixels, &p.pitch);
	memcpy(p.pixels, converted_surface -> pixels, size.x * size.y * sizeof(Uint32));
	SDL_UnlockTexture(p.texture);
	SDL_FreeSurface(converted_surface);

	return p;
}

#define deinit_sprite(sprite) SDL_DestroyTexture(sprite.texture);
#define deinit_psprite(p) SDL_DestroyTexture(p.texture);
