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

#define MAX(a, b) ((a) > (b) ? (a) : (b))

// https://aryamansharda.medium.com/image-filters-gaussian-blur-eb36db6781b1
SDL_Surface* gaussian_blur(SDL_Surface* const image, const int radius) {
	const double sigma = MAX((radius / 2.0), 1.0);
	const int kernel_size = 2 * radius + 1;
	double kernel[kernel_size][kernel_size], sum = 0.0;

	DEBUG(kernel_size, d);

	memset(kernel, 0, kernel_size * kernel_size * sizeof(double));

	for (int y = -radius; y <= radius; y++) {
		for (int x = -radius; x <= radius; x++) {
			const double
				exponent_numerator = -(x * x + y * y),
				exponent_denominator = 2 * sigma * sigma;

			const double e_expression = pow(M_E, exponent_numerator / exponent_denominator);
			const double kernel_value = e_expression / (2 * M_PI * sigma * sigma);

			kernel[y + radius][x + radius] = kernel_value;
			sum += kernel_value;
		}
	}

	for (int y = 0; y < kernel_size; y++) {
		for (int x = 0; x < kernel_size; x++)
			kernel[y][x] /= sum;
	}

	printf("kernel = {\n\t{%lf, %lf, %lf},\n\t{%lf, %lf, %lf},\n\t{%lf, %lf, %lf}\n}\n",
		kernel[0][0], kernel[0][1], kernel[0][2],
		kernel[1][0], kernel[1][1], kernel[1][2],
		kernel[2][0], kernel[2][1], kernel[2][2]);

	//////////

	const ivec image_size = {image -> w, image -> h};

	SDL_Surface* const blurred = SDL_CreateRGBSurfaceWithFormat(0, image_size.x, image_size.y, 32, PIXEL_FORMAT);
	const SDL_PixelFormat* const format = blurred -> format;
	const int bpp = format -> BytesPerPixel;

	SDL_LockSurface(image);
	SDL_LockSurface(blurred);

	for (int y = radius; y < image_size.y - radius; y++) {
		for (int x = radius; x < image_size.x - radius; x++) {
			double blurred_pixel[4] = {0.0, 0.0, 0.0, 0.0};

			for (int kernel_y = -radius; kernel_y <= radius; kernel_y++) {
				for (int kernel_x = -radius; kernel_x <= radius; kernel_x++) {
					const double kernel_value = kernel[kernel_y + radius][kernel_x + radius];

					const Uint32 orig_pixel = *read_surface_pixel(image, x - kernel_x, y - kernel_y, bpp);

					byte r, g, b, a;
					SDL_GetRGBA(orig_pixel, format, &r, &g, &b, &a);
					blurred_pixel[0] += r * kernel_value;
					blurred_pixel[1] += g * kernel_value;
					blurred_pixel[2] += b * kernel_value;
					blurred_pixel[3] += a * kernel_value;
					/*
					printf("blurred_pixel = {%lf, %lf, %lf, %lf}\n",
						blurred_pixel[0], blurred_pixel[1], blurred_pixel[2], blurred_pixel[3]);
					*/
				}
			}

			/*
			printf("blurred_pixel = {%lf, %lf, %lf, %lf}\n",
				blurred_pixel[0], blurred_pixel[1], blurred_pixel[2], blurred_pixel[3]);
			*/

			*read_surface_pixel(blurred, x, y, bpp) = SDL_MapRGBA(format,
				(byte) blurred_pixel[0], (byte) blurred_pixel[1], blurred_pixel[2], blurred_pixel[3]);
		}
	}

	SDL_UnlockSurface(image);
	SDL_UnlockSurface(blurred);

	return blurred;
}

void gauss_blur_test(void) {
	SDL_Surface* const unconverted_image = SDL_LoadBMP("assets/walls/rug_2.bmp");
	SDL_Surface* const image = SDL_ConvertSurfaceFormat(unconverted_image, PIXEL_FORMAT, 0);
	SDL_FreeSurface(unconverted_image);

	SDL_Surface* const blurred = gaussian_blur(image, 50);
	SDL_SaveBMP(blurred, "out.bmp");
	SDL_FreeSurface(blurred);

	exit(0);
}
