Sprite init_sprite(const char* const path) { // put in other file
	SDL_Surface* const surface = SDL_LoadBMP(path);
	if (surface == NULL) FAIL("Could not load a surface with the path of %s\n", path);

	const Sprite sprite = {SDL_CreateTextureFromSurface(screen.renderer, surface), {surface -> w, surface -> h}};

	/*
	const SDL_Texture* const prev_render_target = SDL_GetRenderTarget(screen.renderer);
	SDL_SetRenderTarget(screen.renderer, NULL);
	SDL_SetRenderTarget(screen.renderer, prev_render_target);
	*/

	SDL_FreeSurface(surface);
	return sprite;
}

inlinable void deinit_sprite(const Sprite sprite) {
	SDL_DestroyTexture(sprite.texture);
}
