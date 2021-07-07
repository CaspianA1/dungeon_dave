Sprite init_sprite(const char* const path) {
	SDL_Surface* const unconverted_surface = SDL_LoadBMP(path);
	if (unconverted_surface == NULL) FAIL("Could not load a sprite of path %s\n", path);

	/*
	SDL_Surface* const converted_surface = SDL_ConvertSurface(unconverted_surface, screen.pixel_format, 0);
	SDL_PixelFormat* foo = SDL_GetWindowPixelFormat(screen.window);
	*/

	SDL_Surface* const converted_surface = SDL_ConvertSurfaceFormat(unconverted_surface, SDL_GetWindowPixelFormat(screen.window), 0);

	if (converted_surface == NULL) FAIL("Could not convert a sprite's surface type: %s\n", path);
	SDL_FreeSurface(unconverted_surface);
	SDL_LockSurface(converted_surface);

	return (Sprite) {
		converted_surface,
		SDL_CreateTextureFromSurface(screen.renderer, converted_surface)
	};
}

inlinable void deinit_sprite(const Sprite sprite) {
	SDL_UnlockSurface(sprite.surface);
	SDL_FreeSurface(sprite.surface);
	SDL_DestroyTexture(sprite.texture);
}
