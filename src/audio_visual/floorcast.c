inlinable vec vec_tex_offset(const vec pos, const int tex_size) {
	return vec_fill(tex_size) * (pos - _mm_round_pd(pos, _MM_FROUND_TRUNC));
}


/*
idea:
1. turn the 32-bit int into 4 8-bit ints,
2. simd multiply the first three by the shade
3. make that vector one number

xmm registers store 4 32-bit floating-point numbers
need a way to take each byte out of a 32-bit number and put each part in a section of an xmm register
*/

void print_bits(Uint32 val) {
	for (int i = 31; 0 <= i; i--)
		printf("%c", (val & (1 << i)) ? '1' : '0');
	putchar('\n');
}

#ifdef SHADING_ENABLED

Uint32 shade_ARGB_pixel(const Uint32 pixel, const double dist, const vec hit) {
	const double shade = calculate_shade(settings.proj_dist / dist, hit);
	const byte r = (byte) (pixel >> 16) * shade, g = (byte) (pixel >> 8) * shade, b = (byte) pixel * shade;
	return 0xFF000000 | (r << 16) | (g << 8) | b; // this line + calculate_shade are big slowdowns
}

// https://stackoverflow.com/questions/58895781/extracting-bytes-from-a-32-bits-number
Uint32 shader_2(const Uint32 pixel, const double dist, const vec hit) {
	(void) dist;
	(void) hit;
	return pixel;

	/*
	print_bits(pixel);
	const double shade = 0.8;
	DEBUG(shade, lf);
	const byte r = (byte) (pixel >> 16) * shade, g = (byte) (pixel >> 8) * shade, b = (byte) pixel * shade;
	const Uint32 result = 0xFF000000 | (r << 16) | (g << 8) | b;
	print_bits(result);
	printf("---\n");
	return result;
	*/

	/*
	11111111 11011001 10100000 01100110
	shade = 0.800000
	11111111 10101101 10000000 01010001

	the shaded part = 101011011000000001010001

	01100110 * 0.8 = 1010001 (didn't equal 01010001?)
	*/


	/*
	printf("Pixel: ");
	print_bits(pixel);

	const Uint32 rgb = (pixel >> 8);
	printf("RGB: ");	
	print_bits(rgb);

	const Uint32 shaded = rgb * 0.8;	

	printf("Result: ");
	const Uint32 result = 0xFF000000 | shaded;
	print_bits(result);
	printf("---\n");

	return result;
	*/
}

#endif

PSprite p;
void draw_from_hit(const vec hit, const double dist, const int screen_x, Uint32* const pixbuf_row) {
	const vec offset = vec_tex_offset(hit, p.size);
	Uint32 pixel = *(read_texture_row(p.pixels, p.pitch, offset[1]) + (long) offset[0]);

	#ifdef SHADING_ENABLED
	pixel = shade_ARGB_pixel(pixel, dist, hit);
	// pixel = shader_2(pixel, dist, hit);
	#else
	(void) dist;
	#endif

	for (int x = screen_x; x < screen_x + settings.ray_column_width; x++)
		*(pixbuf_row + x) = pixel;
}

void fast_affine_floor(const vec pos, const double full_jump_height,
	const double pace, double y_shift, const int y_pitch) {

	const double opp_h = 0.5 + full_jump_height / settings.proj_dist;
	if (y_shift < 0.0) y_shift = 0.0;

	/*
	Uint32* pixbuf_row = (Uint32*) ((Uint8*) screen.pixels + (int) y_shift * screen.pixel_pitch); 
	const Uint32 pixbuf_row_step = screen.pixel_pitch / sizeof(Uint32); // 1000
	*/

	// `y_shift - pace` may go outside the map boundaries; limit this domain
	for (int y = y_shift - pace; y < settings.screen_height - pace; y++) {
	// for (int y = y_shift - pace; y < settings.screen_height - pace; y++, pixbuf_row += pixbuf_row_step) {
		const int row = y - settings.half_screen_height - y_pitch + 1;
		if (row == 0) continue;

		const double straight_dist = opp_h / row * settings.proj_dist;

		const int pace_y = y + pace;
		Uint32* const pixbuf_row = read_texture_row(screen.pixels, screen.pixel_pitch, pace_y);

		/*
		if (cmp_pixbuf_row == pixbuf_row) {
			printf("Equal\n");
		}
		else {
			printf("Not equal\n");
		}
		*/

		for (int screen_x = 0; screen_x < settings.screen_width; screen_x += settings.ray_column_width) {
			const BufferVal buffer_val = val_buffer[screen_x];
			if (buffer_val.wall_bottom >= pace_y + 1) continue;

			const double actual_dist = straight_dist / (double) buffer_val.cos_beta;
			const vec hit = vec_line_pos(pos, buffer_val.dir, actual_dist);

			#ifdef PLANAR_MODE

			if (hit[0] < 1.0 || hit[1] < 1.0 || hit[0] > current_level.map_size.x - 1.0
				|| hit[1] > current_level.map_size.y - 1.0) continue;

			#else

			const byte wall_point = map_point(current_level.wall_data, hit[0], hit[1]);
			if (current_level.get_point_height(wall_point, hit)) continue;

			#endif

			draw_from_hit(hit, actual_dist, screen_x, pixbuf_row);
		}
	}
}

#ifdef PLANAR_MODE

void fill_val_buffers_for_planar_mode(const double angle_degrees) {
	const double player_angle = to_radians(angle_degrees);

	for (int screen_x = 0; screen_x < settings.screen_width; screen_x += settings.ray_column_width) {
		const double theta = atan((screen_x - settings.half_screen_width) / settings.proj_dist) + player_angle;
		update_val_buffers(screen_x, 0.0, cos(player_angle - theta), 0.0f, (vec) {cos(theta), sin(theta)});
	}
}

#endif
