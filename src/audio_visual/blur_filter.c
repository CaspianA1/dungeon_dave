typedef __v4si color4;

inlinable Uint32* read_surface_pixel(const SDL_Surface* const surface, const int x, const int y, const int bpp) {
	return (Uint32*) ((Uint8*) surface -> pixels + y * surface -> pitch + x * bpp);
}

inlinable Uint32 box_blur_pixel(const SDL_Surface* const image, const SDL_PixelFormat* const format,
	const ivec image_size, const int bpp, const int x, const int y, const int blur_size) {

	color4 sum = {0, 0, 0, 0};
	int blur_sum_factor = 0;

	for (int py = -blur_size; py <= blur_size; py++) {

		const int y1 = y + py;
		if (y1 < 0) continue;
		else if (y1 >= image_size.y) break;

		for (int px = -blur_size; px <= blur_size; px++) {

			const int x1 = x + px;
			if (x1 < 0) continue;
			else if (x1 >= image_size.x) break;

			const Uint32 pixel = *read_surface_pixel(image, x1, y1, bpp);

			byte r, g, b, a;
			SDL_GetRGBA(pixel, format, &r, &g, &b, &a);
			sum += (color4) {r, g, b, a};
			blur_sum_factor++;
		}
	}

	if (blur_sum_factor == 0) blur_sum_factor = 1;

	return SDL_MapRGBA(format,
		sum[0] / blur_sum_factor,
		sum[1] / blur_sum_factor,
		sum[2] / blur_sum_factor,
		sum[3] / blur_sum_factor);
}

void box_blur_image_portion(SDL_Surface* const image, SDL_Rect crop, const int blur_size) {
	const ivec src_size = {image -> w, image -> h};

	SDL_Surface* const blurred_crop = SDL_CreateRGBSurfaceWithFormat(0, crop.w, crop.h, 32, PIXEL_FORMAT);
	SDL_LockSurface(blurred_crop);
	SDL_LockSurface(image);

	const SDL_PixelFormat* const format = image -> format;
	const int bpp = format -> BytesPerPixel;
 	for (int y = crop.y; y < crop.y + crop.h; y++) {
		for (int x = crop.x; x < crop.x + crop.w; x++)
			*read_surface_pixel(blurred_crop, x - crop.x, y - crop.y, bpp) =
				box_blur_pixel(image, format, src_size, bpp, x, y, blur_size);
	}

	SDL_UnlockSurface(blurred_crop);
	SDL_UnlockSurface(image);

	/*
	system("mkdir imgs");
	static byte id = 0;
	char buf[16];
	sprintf(buf, "imgs/out_%d.bmp", id);
	SDL_SaveBMP(blurred_crop, buf);
	id++;
	*/

	SDL_BlitSurface(blurred_crop, NULL, image, &crop);
	SDL_FreeSurface(blurred_crop);
}

void box_blur_test(void) {
	SDL_Surface* const unconverted_image = SDL_LoadBMP("assets/walls/mesa.bmp");
	SDL_Surface* const image = SDL_ConvertSurfaceFormat(unconverted_image, PIXEL_FORMAT, 0);
	SDL_FreeSurface(unconverted_image);

	box_blur_image_portion(image, (SDL_Rect) {0, 0, image -> w, image -> h}, 3);
	SDL_SaveBMP(image, "out.bmp");
	SDL_FreeSurface(image);

	exit(0);
}

//////////

void gaussian_blur(SDL_Surface* const image) {
	const int kernel[3][3] = {
		{1, 2, 1},
		{2, 4, 2},
		{1, 2, 1}
	};

	(void) kernel;

	SDL_LockSurface(image);
	SDL_UnlockSurface(image);
}
