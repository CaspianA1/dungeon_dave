SDL_Surface* load_surface(const char* const path) {
	SDL_Surface* surface = SDL_LoadBMP(path);
	if (surface == NULL) FAIL("Could not load a surface with the path of %s\n", path);

	SDL_Surface* const converted_surface = SDL_ConvertSurfaceFormat(surface, PIXEL_FORMAT, 0);
	if (converted_surface == NULL) FAIL("Could not convert a surface to the default pixel format: %s\n", path);
	SDL_FreeSurface(surface);
	return converted_surface;
}

/*
Texture filtering:

If OpenGL filtering enabled:
	If a wall or a thing: enable trilinear filtering with mipmapping
	If overlay, no mipmapping
	If a skybox, enable linear filtering via OpenGL (same as via SDL)

Else:
	If a wall, enable software mipmapping
	If a thing or overlay, no mipmapping
	If a skybox, enable linear filtering via SDL
*/

Sprite init_sprite(const char* const path, const DrawableType drawable_type) {
	SDL_Surface* surface = load_surface(path);

	Sprite sprite;

	#ifdef OPENGL_TEXTURE_FILTERING
	create_filtered_texture(surface, &sprite, drawable_type);
	#else

	switch (drawable_type) {
		case D_Wall: {
			SDL_Surface* const mipmap = load_mipmap(surface, &sprite.max_mipmap_depth);
			if (mipmap == NULL) {
				FAIL("The sprite with the path %s must have dimensions that are powers of two\n", path);
			}

			SDL_FreeSurface(surface); // free the previous surface
			surface = mipmap; // replace it with the mipmap
			break;
		}
		case D_Skybox:
			SDL_SetHintWithPriority(SDL_HINT_RENDER_SCALE_QUALITY, "1", SDL_HINT_OVERRIDE); // linear filtering
			break;
		default:
			break;
	}

	sprite.texture = SDL_CreateTextureFromSurface(screen.renderer, surface);

	if (drawable_type == D_Skybox) // reset linear filtering hint
		SDL_SetHintWithPriority(SDL_HINT_RENDER_SCALE_QUALITY, "0", SDL_HINT_OVERRIDE);

	#endif

	sprite.size = (ivec) {surface -> w, surface -> h};
	SDL_FreeSurface(surface);
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
