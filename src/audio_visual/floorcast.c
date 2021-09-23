inlinable vec vec_tex_offset(const vec pos, const int tex_size) {
	return vec_fill(tex_size) * (pos - vec_trunc(pos));
}

inlinable Uint32* read_texture_row(const void* const pixels, const int pixel_pitch, const int y) {
	return (Uint32*) ((Uint8*) pixels + y * pixel_pitch);
}

#ifdef SHADING_ENABLED

// this fn is fast b/c it depends on no floating-point operations
inlinable Uint32 shade_ARGB_pixel(const Uint32 pixel, const byte shade) {
	int r = (byte) (pixel >> 16), g = (byte) (pixel >> 8), b = (byte) pixel;

	r *= shade;
	g *= shade;
	b *= shade;

	r /= 255; // the divs by 255 are optimized out by the compiler as bitwise ops
	g /= 255;
	b /= 255;

	return 0xFF000000 | (r << 16) | (g << 8) | b;
}

#endif

static PixSprite ground;
void floorcast(const byte floor_height, const int horizon_line, int start_y, const int end_y,
	const vec pos, const double p_height) {

	const double eye_height = (p_height - floor_height) + 0.5;
	if (eye_height < 0.0) return;
	else if (start_y < 0) start_y = 0;

	for (int screen_y = start_y; screen_y < end_y; screen_y++) {
		Uint32* const pixbuf_row = read_texture_row(screen.pixels, screen.pixel_pitch, screen_y);

		const int row = screen_y - horizon_line + 1;
		const double straight_dist = eye_height / row * settings.proj_dist;

		for (int screen_x = 0; screen_x < settings.screen_width; screen_x += settings.ray_column_width) {
			/* The remaining bottlenecks:
			Checking for statemap bits will not be needed with a visplane system -> speedup
			Also, that means that the statemap won't be needed anymore -> speedup
			Checking for out-of-bound hits won't be needed either -> speedup
			Checking for wall points that do not equal the floor height won't be needed either -> speedup
			Also, with a visplane system, less y and x coordinates will be iterated over -> speedup */

			#ifndef PLANAR_MODE
			if (get_statemap_bit(occluded_by_walls, screen_x, screen_y)) continue;
			#endif

			const FloorcastBufferVal floorcast_buffer_val = floorcast_val_buffer[screen_x];
			const double actual_dist = straight_dist * (double) floorcast_buffer_val.one_over_cos_beta;
			const vec hit = vec_line_pos(pos, floorcast_buffer_val.dir, actual_dist);

			if (hit[0] < 1.0 || hit[1] < 1.0 || hit[0] > current_level.map_size.x - 1.0
				|| hit[1] > current_level.map_size.y - 1.0) continue;

			#ifndef PLANAR_MODE
			const byte wall_point = *map_point(current_level.wall_data, hit[0], hit[1]);
			if (current_level.get_point_height(wall_point, hit) != floor_height) continue;
			#endif

			const vec offset = vec_tex_offset(hit, ground.size);
			Uint32 pixel = ground.pixels[(long) ((long) offset[1] * ground.size + offset[0])];

			#ifdef SHADING_ENABLED
			pixel = shade_ARGB_pixel(pixel, calculate_shade(settings.proj_dist / actual_dist, hit));
			#endif

			for (int x = 0; x < settings.ray_column_width; x++) pixbuf_row[x + screen_x] = pixel;
		}
	}
}

#ifdef PLANAR_MODE

void fill_val_buffers_for_planar_mode(const double angle_degrees) {
	const double p_angle = to_radians(angle_degrees);

	for (int screen_x = 0; screen_x < settings.screen_width; screen_x += settings.ray_column_width) {
		const double theta = atan((screen_x - settings.half_screen_width) / settings.proj_dist) + p_angle;
		update_buffers(screen_x, 0.0f, cosf(p_angle - theta), (vec) {cos(theta), sin(theta)});
	}
}

#endif

////////// The two structs + the two functions below manage parallel floorcasting

typedef struct {
	const byte floor_height;
	const vec pos;
	const double p_height;
	const int horizon_line;
} ImmutFloorcastCallerParams;

typedef struct {
	const ImmutFloorcastCallerParams* const immut;
	const int start_y, end_y;
} FloorcastCallerParams;

int floorcast_caller(void* const data) {
	const FloorcastCallerParams params = *(FloorcastCallerParams*) data;
	const ImmutFloorcastCallerParams* const i_params = params.immut;

	floorcast(i_params -> floor_height, i_params -> horizon_line,
		params.start_y, params.end_y, i_params -> pos, i_params -> p_height);

	return 0;
}

void parallel_floorcast(const byte floor_height, const vec pos, const double p_height, const int horizon_line) {
	const int cast_start = horizon_line, cast_end = settings.screen_height;
	const int y_step = (double) (cast_end - cast_start) / FLOORCAST_THREADS;

	const ImmutFloorcastCallerParams immut_params = {
		.floor_height = floor_height, .pos = pos, .p_height = p_height, .horizon_line = horizon_line
	};

	SDL_Thread* threads[FLOORCAST_THREADS];
	FloorcastCallerParams params[FLOORCAST_THREADS];

	// vertical y-ranges are drawn in parallel here
	for (byte i = 0; i < FLOORCAST_THREADS; i++) {
		const int drawer_start = cast_start + y_step * i;
		const int drawer_end = (i == FLOORCAST_THREADS - 1) ? cast_end : drawer_start + y_step;

		const FloorcastCallerParams copy_params = {&immut_params, drawer_start, drawer_end};
		memcpy(params + i, &copy_params, sizeof(FloorcastCallerParams));
		threads[i] = SDL_CreateThread(floorcast_caller, "floorcast_thread_instance", params + i);
	}

	for (byte i = 0; i < FLOORCAST_THREADS; i++) SDL_WaitThread(threads[i], 0);
}
