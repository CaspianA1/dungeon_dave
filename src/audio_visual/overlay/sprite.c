Sprite init_sprite(const char* const path) { // put in other file
	SDL_Surface* surface = SDL_LoadBMP(path);
	surface = SDL_ConvertSurfaceFormat(surface, PIXEL_FORMAT, 0);

	if (surface == NULL) FAIL("Could not load a surface with the path of %s\n", path);
	const Sprite sprite = {SDL_CreateTextureFromSurface(screen.renderer, surface), {surface -> w, surface -> h}};

	SDL_FreeSurface(surface);

	return sprite;
}

inlinable void deinit_sprite(const Sprite sprite) {
	SDL_DestroyTexture(sprite.texture);
}
