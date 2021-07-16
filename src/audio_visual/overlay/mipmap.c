const byte max_mipmap_depth = 4;
const double max_mipmap_dist = 30.0;

SDL_Rect get_mipmap_crop(const ivec size, const byte depth_offset) {
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

	SDL_Rect dest = {.w = surface -> w, .h = surface -> h};
	for (byte i = 0; i < max_mipmap_depth; i++) {
		if (i == 0) dest.x = 0;
		else dest.x = surface -> w;

		if (i <= 1) dest.y = 0;
		else dest.y += surface -> h >> (i - 1);

		SDL_BlitScaled(surface, NULL, mipmap, &dest);

		void apply_gaussian_blur(SDL_Surface* const, const SDL_Rect);
		apply_gaussian_blur(mipmap, dest);

		dest.w >>= 1;
		dest.h >>= 1;
	}

	return mipmap;
}

inlinable Uint32* read_surface_pixel(const SDL_Surface* const surface, const int x, const int y, const int bpp) {
	return (Uint32*) ((Uint8*) surface -> pixels + y * surface -> pitch + x * bpp);
}

/*
sum = {82.556777, 76.857143, 68.890110}
kernel:
---
0.003663, 0.014652, 0.025641, 0.014652, 0.003663, 
0.014652, 0.058608, 0.095238, 0.058608, 0.014652, 
0.025641, 0.095238, 0.150183, 0.095238, 0.025641, 
0.014652, 0.058608, 0.095238, 0.058608, 0.014652, 
0.003663, 0.014652, 0.025641, 0.014652, 0.003663, 
---
sum = {82.556777, 76.857143, 68.890110}
kernel:
---
0.003663, 0.014652, 0.025641, 0.014652, 0.003663, 
0.014652, 0.058608, 0.095238, 0.058608, 0.014652, 
0.025641, 0.095238, 0.150183, 0.095238, 0.025641, 
0.014652, 0.058608, 0.095238, 0.058608, 0.014652, 
0.003663, 0.014652, 0.025641, 0.014652, 0.003663, 
---
*/

// why does each row look the same?
Uint32 get_gaussian_for_pixel(const SDL_Surface* const mipmap, const int x, const int y) {
	const double sigma = 1.0 / 273.0; // std dev of this gaussian distribution

	const int kernel[5][5] = {
		{1, 4, 7, 4, 1},
		{4, 16, 26, 16, 4},
		{7, 26, 41, 26, 7},
		{4, 16, 26, 16, 4},
		{1, 4, 7, 4, 1}
	};

	static byte first = 1;
	if (first) {
		printf("kernel:\n---\n");
		for (int y = 0; y < 5; y++) {
			for (int x = 0; x < 5; x++)
				printf("%lf, ", kernel[y][x] * sigma);
			putchar('\n');
		}
		printf("---\n");
	}

	double sum[3] = {0.0, 0.0, 0.0};

	const SDL_PixelFormat* const format = mipmap -> format;

	for (int kernel_y_offset = -2; kernel_y_offset <= 2; kernel_y_offset++) {
		const int mipmap_y_offset = y + kernel_y_offset;
		if (mipmap_y_offset < 0 || mipmap_y_offset >= mipmap -> h) continue;

		Uint32* const mipmap_row = (Uint32*) ((Uint8*) mipmap -> pixels + mipmap_y_offset * mipmap -> pitch);

		for (int kernel_x_offset = -2; kernel_x_offset <= 2; kernel_x_offset++) {
			const int mipmap_x_offset = x + kernel_x_offset;
			if (mipmap_x_offset < 0 || mipmap_x_offset >= mipmap -> w) continue;

			const Uint32* const pixel = mipmap_row + kernel_x_offset * format -> BytesPerPixel;
			const double kernel_val = kernel[kernel_y_offset + 2][kernel_x_offset + 2] * sigma;

			byte r, g, b;
			SDL_GetRGB(*pixel, mipmap -> format, &r, &g, &b);
			sum[0] += r * kernel_val;
			sum[1] += g * kernel_val;
			sum[2] += b * kernel_val;
		}
	}
	if (first) {
		printf("sum = {%lf, %lf, %lf}\n", sum[0], sum[1], sum[2]);
		first = 0; // write to a new image?
	}

	return SDL_MapRGB(format, sum[0], sum[1], sum[2]);
	// return 0xFF000000 | (210 << 16) | (180 << 8) | 140;
}

/* https://aryamansharda.medium.com/image-filters-gaussian-blur-eb36db6781b1
https://computergraphics.stackexchange.com/questions/39/how-is-gaussian-blur-implemented */
void apply_gaussian_blur(SDL_Surface* mipmap, const SDL_Rect depth_crop) {
	SDL_Surface* const blurred_dest = SDL_CreateRGBSurfaceWithFormat(0, depth_crop.w, depth_crop.h, 32, PIXEL_FORMAT);

	const int bpp = mipmap -> format -> BytesPerPixel;
	for (int y = depth_crop.y, dest_y = 0; y < depth_crop.y + depth_crop.h; y++, dest_y++) {
		for (int x = depth_crop.x, dest_x = 0; x < depth_crop.x + depth_crop.w; x++, dest_x++)
			*read_surface_pixel(blurred_dest, dest_x, dest_y, bpp) = get_gaussian_for_pixel(mipmap, x, y);
	}

	static byte first = 1;
	if (first) {
		SDL_SaveBMP(blurred_dest, "out.bmp");
		first = 0;
	}
}
