Sprite init_sprite(const char* const path) { // put in other file
	SDL_Surface* const surface = SDL_LoadBMP(path);
	if (surface == NULL) FAIL("Could not load a surface with the path of %s\n", path);

	const Sprite sprite = {SDL_CreateTextureFromSurface(screen.renderer, surface), {surface -> w, surface -> h}};
	SDL_FreeSurface(surface);
	return sprite;
}

PSprite init_psprite(const char* const path) {
	SDL_Surface* const surface = SDL_LoadBMP(path);
	if (surface == NULL) FAIL("Could not load a surface with the path of %s\n", path);

	const ivec size = {surface -> w, surface -> h};

	SDL_Texture* const texture = SDL_CreateTexture(screen.renderer, PIXEL_FORMAT,
		SDL_TEXTUREACCESS_STREAMING, size.x, size.y);

	PSprite p;

	SDL_LockTexture(texture, NULL, &p.pixels, &p.pitch);
	memcpy(p.pixels, surface -> pixels, size.x * size.y * sizeof(Uint32));
	SDL_UnlockTexture(texture);
	SDL_FreeSurface(surface);

	p.sprite = (Sprite) {texture, size};

	return p;
}

#define deinit_sprite(sprite) SDL_DestroyTexture(sprite.texture);
#define deinit_pixelwise_sprite(sprite) SDL_DestroyTexture(sprite.texture);

inlinable Uint32* read_texture_row(const void* const pixels, const int pixel_pitch, const int y) {
	return (Uint32*) ((Uint8*) pixels + y * pixel_pitch);
}
