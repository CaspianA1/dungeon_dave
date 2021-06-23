Sprite init_sprite(const char* const path) {
	SDL_Surface* const unconverted_surface = SDL_LoadBMP(path);
	if (unconverted_surface == NULL) FAIL("Could not load a sprite of path %s\n", path);

	SDL_Surface* const converted_surface = SDL_ConvertSurface(unconverted_surface, screen.pixel_format, 0);
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

// offset is vertical
inlinable void draw_column(const Sprite sprite, const VectorF hit,
	const int offset, const int slice_h, const double shade_h, const SDL_FRect* const dest) {
	/* slice_h pertains to src crop. shade_h pertains to shading.
	for partially obscured walls, need shade h. */

	#ifndef SHADING_ENABLED
	(void) shade_h;
	(void) hit;
	#endif

	const byte shade = 255 * calculate_shade(
		(shade_h == -1) ? (double) dest -> h : shade_h, hit);

	SDL_SetTextureColorMod(sprite.texture, shade, shade, shade);

	const SDL_Rect slice = {
		offset, 0, 1,
		(slice_h == -1) ? sprite.surface -> h : slice_h
	};	

	SDL_RenderCopyF(screen.renderer, sprite.texture, &slice, dest);
}
