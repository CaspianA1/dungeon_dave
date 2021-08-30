inlinable Uint32* read_surface_pixel(const SDL_Surface* const surface, const int x, const int y, const int bpp) {
	return (Uint32*) ((Uint8*) surface -> pixels + y * surface -> pitch + x * bpp);
}

void box_blur_image_portion(SDL_Surface* const image, SDL_Rect crop, const int blur_size) { // box blur
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

			// TODO: figure out why the edges are close to black only when this is called by load_mipmap

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
	SDL_Surface* const unconverted_image = SDL_LoadBMP("assets/walls/dune.bmp");
	SDL_Surface* const image = SDL_ConvertSurfaceFormat(unconverted_image, PIXEL_FORMAT, 0);
	SDL_FreeSurface(unconverted_image);

	box_blur_image_portion(image, (SDL_Rect) {0, 0, image -> w, image -> h}, 3);
	SDL_SaveBMP(image, "out.bmp");
	SDL_FreeSurface(image);

	exit(0);
}
