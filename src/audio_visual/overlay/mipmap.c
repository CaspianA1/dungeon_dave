static const byte max_mipmap_depth = 4;
static const double max_mipmap_dist = 30.0;

static SDL_Rect get_mipmap_crop(const ivec size, const byte depth_offset) {
	const int orig_w = size.x * 2 / 3;

	SDL_Rect dest = {
		.x = (depth_offset == 0) ? 0 : orig_w,
		.y = 0,
		.w = orig_w >> depth_offset,
		.h = size.y >> depth_offset
	};

	for (byte i = 2; i < depth_offset + 1; i++)
		dest.y += size.y >> (i - 1);

	return dest;
}

inlinable SDL_Rect get_mipmap_crop_from_dist(const ivec size, const double dist) {
	byte depth_offset = dist / max_mipmap_dist * max_mipmap_depth;
	if (depth_offset >= max_mipmap_depth) depth_offset = max_mipmap_depth - 1;
	return get_mipmap_crop(size, depth_offset);
}

SDL_Surface* load_mipmap(SDL_Surface* const surface) {
	SDL_Surface* const mipmap = SDL_CreateRGBSurfaceWithFormat(0,
		surface -> w + (surface -> w >> 1), surface -> h, 32, PIXEL_FORMAT);

	void blur_image_portion(SDL_Surface* const, SDL_Rect, const int);

	SDL_Rect dest = {.w = surface -> w, .h = surface -> h};
	for (byte i = 0; i < max_mipmap_depth; i++) {
		if (i == 0) dest.x = 0;
		else dest.x = surface -> w;

		if (i <= 1) dest.y = 0;
		else dest.y += surface -> h >> (i - 1);

		SDL_BlitScaled(surface, NULL, mipmap, &dest);
		blur_image_portion(mipmap, dest, i / 2);

		dest.w >>= 1;
		dest.h >>= 1;
	}

	return mipmap;
}

//////////

inlinable Uint32* read_surface_pixel(const SDL_Surface* const surface, const int x, const int y, const int bpp) {
	return (Uint32*) ((Uint8*) surface -> pixels + y * surface -> pitch + x * bpp);
}

void blur_image_portion(SDL_Surface* const image, SDL_Rect crop, const int blur_size) { // box blur
	typedef __v4si color4;

	const int src_w = image -> w, src_h = image -> h;
	SDL_Surface* const blurred_crop = SDL_CreateRGBSurfaceWithFormat(0, crop.w, crop.h, 32, PIXEL_FORMAT);
	SDL_LockSurface(blurred_crop);
	SDL_LockSurface(image);

	const SDL_PixelFormat* const format = image -> format;
	const int bpp = format -> BytesPerPixel;
 	for (int y = crop.y; y < crop.y + crop.h; y++) {
		for (int x = crop.x; x < crop.x + crop.w; x++) {
			color4 sum = {0, 0, 0, 0};
			int blur_sum_factor = 0;
			for (int py = -blur_size; py <= blur_size; py++) {
				for (int px = -blur_size; px <= blur_size; px++) {
					const int x1 = x + px, y1 = y + py;
					if (x1 < 0 || y1 < 0) continue;
					else if (x1 >= src_w || y1 >= src_h) break;

					const Uint32 pixel = *read_surface_pixel(image, x1, y1, bpp);

					byte r, g, b, a;
					SDL_GetRGBA(pixel, format, &r, &g, &b, &a);
					sum += (color4) {r, g, b, a};
	
					blur_sum_factor++;
                }
			}

			if (blur_sum_factor == 0) blur_sum_factor = 1;
			const byte out[4] = {
				sum[0] / blur_sum_factor,
				sum[1] / blur_sum_factor,
				sum[2] / blur_sum_factor,
				sum[3] / blur_sum_factor
			};

			const Uint32 blurred_pixel = SDL_MapRGBA(format, out[0], out[1], out[2], out[3]);
			*read_surface_pixel(blurred_crop, x - crop.x, y - crop.y, bpp) = blurred_pixel;
		}
	}

	SDL_UnlockSurface(blurred_crop);
	SDL_UnlockSurface(image);

	SDL_BlitSurface(blurred_crop, NULL, image, &crop);
	SDL_FreeSurface(blurred_crop);
}

void blur_test(void) {
	SDL_Surface* const unconverted_image = SDL_LoadBMP("assets/walls/smooth_viney_bricks.bmp");
	SDL_Surface* const image = SDL_ConvertSurfaceFormat(unconverted_image, PIXEL_FORMAT, 0);
	SDL_FreeSurface(unconverted_image);

	blur_image_portion(image, (SDL_Rect) {0, 0, image -> w, image -> h}, 2);
	SDL_SaveBMP(image, "out.bmp");
	SDL_FreeSurface(image);

	exit(0);
}
