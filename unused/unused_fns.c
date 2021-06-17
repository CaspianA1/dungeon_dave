#define CLEAR(rend) {SET_COLOR(rend, 0, 0, 0); SDL_RenderClear(rend);}

#define dist_adjustment(rad_theta, player) cos(rad_theta - to_radians(player.angle))

inlinable int is_a_sprite(int num) {
	return num > wall_count;
}

inlinable const double two_decimal_places(double val) {
	return (int) (val * 100 + 0.5) / 100.0f;
}

inlinable void set_color(SDL_Renderer* renderer, const int r, const int g, const int b) {
	SDL_SetRenderDrawColor(renderer, r, g, b, SDL_ALPHA_OPAQUE);
}

inlinable void draw_pixel(const int x, const int y, const int r,const int g, const int b) {
	Uint32* pixel = ((Uint32*) ((Uint8*) screen.pixels + y * screen.pitch)) + x;
	*pixel = 0xFF000000 | (r << 16) | (g << 8) | b;
}

inlinable const double to_degrees(const double radians) {
	return radians * 180.0 / M_PI;
}

inlinable const int tex_ind(const double hit_on_axis, const int size) {
	return (int) ((hit_on_axis - (int) hit_on_axis) * size) & size - 1;
}

inlinable void shade(int* color, const double darkener) {
	// this is for untextured walls
	const int darkened = *color - darkener;
	*color = darkened < 0 ? 0 : darkened;
}

inlinable const double wrap_angle(const double theta) { // keep for now
	if (theta < 0) return theta + two_pi;
	else if (theta > two_pi) return theta - two_pi;
	else return theta;
}

const Uint32 get_surface_pixel(const void* pixels, const int surface_pitch,
						const int bytes_per_pixel, const int x, const int y) {

	const Uint8* pixel = (Uint8*) pixels + y * surface_pitch + x * bytes_per_pixel;

	switch (bytes_per_pixel) {
		case 1: return *pixel;
		case 2: return *(Uint16*) pixel;
		case 3:
			#if SDL_BYTEORDER == SDL_BIG_ENDIAN
				return pixel[0] << 16 | pixel[1] << 8 | pixel[2];
			#else
				return pixel[0] | pixel[1] << 8 | pixel[2] << 16;
			#endif
		case 4: return *(Uint32*) pixel;
		default: 
			return (Uint32) NULL;
	}
}

inlinable int compare_billboard_distances(const void* a, const void* b) {
	return (((Billboard*) b) -> dist_squared - ((Billboard*) a) -> dist_squared);
}

void draw_billboards(const Player player) {
	static Billboard sorted_billboards[billboard_count];
	memcpy(sorted_billboards, billboards, sizeof(sorted_billboards));

	const double player_angle = to_radians(player.angle);

	for (int i = 0; i < billboard_count; i++) {
		Billboard* billboard = &sorted_billboards[i];
		const VectorF delta = {billboard -> pos.x - player.pos.x, billboard -> pos.y - player.pos.y};

		billboard -> beta = atan2(delta.y, delta.x) - player_angle;
		billboard -> dist_squared = (delta.x * delta.x + delta.y * delta.y) * cos(billboard -> beta);
	}

	qsort(sorted_billboards, billboard_count, sizeof(Billboard), compare_billboard_distances);

	for (int i = 0; i < billboard_count; i++) {
		const Billboard billboard = sorted_billboards[i];

		const double abs_billboard_beta = fabs(billboard.beta);

		if (billboard.dist_squared <= 0
			|| doubles_eq(abs_billboard_beta, half_pi)
			|| doubles_eq(abs_billboard_beta, three_pi_over_two)
			|| doubles_eq(abs_billboard_beta, five_pi_over_two))
			continue;

		const double
			dist = sqrt(billboard.dist_squared),
			center_offset = tan(billboard.beta) * screen.projection_distance,
			size = screen.projection_distance / dist;

		const int
			width = billboard.sprite.surface -> w,
			center_x = half_screen_width + center_offset,
			half_size = size / 2;

		int start_x = center_x - half_size, end_x = center_x + half_size;
		if (end_x < 0) continue;

		SDL_Rect pos = {
			0, half_screen_height - half_size + player.pace.screen_offset + player.y_pitch,
			ray_column_width, size
		};

		for (int screen_row = start_x; screen_row < end_x; screen_row++) {
			if (screen_row < 0 || screen_row > screen_width || screen.z_buffer[screen_row] < dist)
				continue;

			const int tex_ind = ((double) (screen_row - start_x) / size) * width;

			pos.x = screen_row;
			draw_scanline(billboard.sprite, tex_ind, &pos);
		}
	}
}


void draw_textured_wall(const CastData cast_data, const SDL_Rect pace_wall) {
	const Sprite wall_sprite = textured_walls[cast_data.point - (wall_count - textured_wall_count + 1)];

	const int max_offset = wall_sprite.surface -> w - 1;

	const VectorF diff = { // many of these are zero a lot
		cast_data.hit.x - floor(cast_data.hit.x),
		cast_data.hit.y - floor(cast_data.hit.y)
	};

	// DEBUG(diff.x, f);

	const VectorI side_collision = {
		close_to_whole(diff.y),
		close_to_whole(diff.x)
	};

	/*
	const int offset =
		(side_collision.x ^ side_collision.y)
			? (int) ((side_collision.x ? diff.x : diff.y) * (max_offset + 1)) & max_offset
			: max_offset;
	*/

	int
		x_side = (int) (diff.x * (max_offset + 1)) % max_offset,
		y_side = (int) (diff.y * (max_offset + 1)) % max_offset,
		offset;

	/*
	if (diff.x > 0.5) y_side = max_offset - y_side;
	if (diff.y > 0.5) x_side = max_offset - x_side;
	*/

	if (x_side ^ y_side)
		offset = side_collision.x ? x_side : y_side;
	else offset = 0;

	// by solving inverted textures, much of this problem should sort itself out

	draw_scanline(wall_sprite, offset, &pace_wall);
}

inlinable const int ray_out_of_bounds(const VectorF ray) {
	return ray.x <= 0.0 || ray.y <= 0.0 || ray.x >= map_width || ray.y >= map_height;
}

inlinable void deinit_animation(Animation animation) {
	deinit_billboard(animation.billboard);
}

inlinable void deinit_animations(Animation* animations, const int amount) {
	for (int i = 0; i < amount; i++)
		deinit_animation(animations[i]);
}

Sprite* init_sprites(const int amount, ...) {
	va_list paths;
	va_start(paths, amount);
	Sprite* sprites = calloc(amount, sizeof(Sprite));

	for (int i = 0; i < amount; i++)
		sprites[i] = init_sprite(va_arg(paths, const char*));

	va_end(paths);
	return sprites;
}

inlinable void deinit_sprites(Sprite* sprites, const int amount) {
	for (int i = 0; i < amount; i++)
		deinit_sprite(sprites[i]);
	free(sprites);
}

inlinable const byte rand_point(const byte* points, const int len) {
	return points[rand() % len];
}

inlinable const VectorI VectorF_round(const VectorF vf) {
	const VectorI vi = {(int) round(vf.x), (int) round(vf.y)};
	return vi;
}

inlinable const int doubles_almost_eq(const double a, const double b) {
	return fabs(a - b) < 0.051;
}

void print_bfs(const Path path) {
	for (int y = 0; y < current_level.map_height; y++) {
		for (int x = 0; x < current_level.map_width; x++) {
			byte color = 210;

			if (current_level.wall_data[y][x]) color = 80;
			else {
				for (int i = 0; i < path.length; i++) {
					const VectorI vertex = path.data[i];
					if (vertex.x == x && vertex.y == y) {
						color = 35;
						goto print_map_value;
					}
				}
			}
			print_map_value: printf("\033[48;5;%dm \033[0m", color);
		}
		putchar('\n');
	}
}

void update_pos_2(__m128d* pos, const __m128d prev_pos, Body2D* body,
	const double rad_theta, const int forward,
	const int backward, const int lstrafe, const int rstrafe) {

	const double curr_time = SDL_GetTicks() / 1000.0;
	const double
		dt = curr_time - body -> last_tick_time,
		cos_theta = cos(rad_theta),
		sin_theta = sin(rad_theta);

	// const double mu_k = 0.12;
	const double accel = body -> a; // - (mu_k * g);

	__m128d v_change = {
		accel * dt * cos_theta,
		accel * dt * sin_theta
	};

	/*
	F_k = mu_k * F_n = mu_k * mg = ma
	mu_k * g = a
	*/

	/*
	if (!forward || backward || lstrafe || rstrafe)
		body -> v = VectorF_memset(0.0);
	*/

	if (forward) body -> v = VectorFF_add(body -> v, v_change);
	if (backward) body -> v = VectorFF_sub(body -> v, v_change);
	if (lstrafe) body -> v[0] += v_change[1], body -> v[1] -= v_change[0];
	if (rstrafe) body -> v[0] -= v_change[1], body -> v[1] += v_change[0];

	// account for neg v here too
	if (body -> v[0] > body -> max_v) body -> v[0] = body -> max_v;
	if (body -> v[1] > body -> max_v) body -> v[1] = body -> max_v;

	// const __m128d point_nine_vec = VectorF_memset(0.90125);
	// body -> v = VectorFF_mul(body -> v, point_nine_vec);

	DEBUG(0.12 * g, lf);
	const __m128d foo = VectorF_memset(0.12 * -g);
	body -> v = VectorFF_mul(body -> v, foo);

	*pos = VectorFF_add(*pos, body -> v);
	body -> last_tick_time = curr_time;

	///
	if (current_level.wall_data[(int) floor((*pos)[1])][(int) floor(prev_pos[0])])
		(*pos)[1] = prev_pos[1];

	if (current_level.wall_data[(int) floor(prev_pos[1])][(int) floor((*pos)[0])])
		(*pos)[0] = prev_pos[0];
	///
}

void draw_skybox(const Skybox skybox, const Player player) {
	const double angle_percentage = player.angle / 360.0;

	const int
		max_width = skybox.sprite.surface -> w,
		max_height = skybox.sprite.surface -> h;

	// 326 seems pretty good for a width, when on 1200 x 700

	const int angle_shift = angle_percentage * max_width;

	// printf("%d\n", angle_shift % max_width);
	const SDL_Rect src = {
		angle_shift,
		0,
		max_width / 4,
		max_height
	},

	dest = {
		0, 0, settings.screen_width,
		settings.half_screen_height
	};

	SDL_RenderCopy(screen.renderer_3D, skybox.sprite.texture, &src, &dest);
}

inlinable const Uint8 get_bits(Uint32 value, const Uint8 offset, const Uint8 n) {
	const size_t max_n = CHAR_BIT * sizeof(Uint32);
	if (offset >= max_n) return 0; // value is padded with infinite zeros on the left
	value >>= offset; // drop offset bits
	if (n >= max_n) return value; // all bits requested
	const Uint32 mask = (1 << n) - 1; // n 1's
	return value & mask;
}

inlinable byte close_to_whole(const double num) {
	double floor_num = floor(num), ceil_num = ceil(num);

	return doubles_eq(num, floor_num) || doubles_eq(num, ceil_num);
}

inlinable byte VectorI_in_range(const double p, const VectorI range) {
	return p >= range.x - small_double_epsilon && p <= range.y + small_double_epsilon;
}

inlinable byte VectorI_in_cell(const VectorF v, const VectorI cell) {
	const VectorI cell_across = {cell.x, cell.x + 1}, cell_down = {cell.y, cell.y + 1};
	return VectorI_in_range(v[0], cell_across) && VectorI_in_range(v[1], cell_down);
}

Uint32 get_surface_pixel_2(const SDL_Surface *surface, const int x, const int y) {
	const int bpp = surface -> format -> BytesPerPixel;
	Uint8* p = (Uint8*) surface -> pixels + y * surface -> pitch + x * bpp;

	switch (bpp) {
		case 1: return *p;
		case 2: return *(Uint16*) p;
		case 3:
			#if SDL_BYTEORDER == SDL_BIGE_ENDIAN
				return p[0] << 16 | p[1] << 8 | p[2];
			#else
				return p[0] | p[1] << 8 | p[2] << 16;
			#endif
			case 4:
				return *(Uint32 *)p;
			default:
				printf("Here\n");
				return 0;
	}
}

void update_enemy(Enemy* enemy, const Player player) {
	const Billboard billboard = enemy -> animations.billboard;
	const double dist = billboard.dist;
	const EnemyDistThresholds dist_thresholds = enemy -> dist_thresholds;

	static const EnemyState threshold_states[3] = {Attacking, Chasing, Idle};
	const double thresholds[3] = {
		dist_thresholds.begin_attacking,
		dist_thresholds.begin_chasing,
		dist_thresholds.max_idle_sound
	};

	byte state_changed = 0;

	if (enemy -> hp < enemy -> hp_to_retreat) {
		enemy -> state = Retreating;
		state_changed = 1;
	}

	else {
		for (byte i = 0; i < 3; i++) {
			const EnemyState other_state = threshold_states[i];
			if (dist < thresholds[i] && enemy -> state != other_state) {
				if (i == 0 || (i == 2 && dist > dist_thresholds.min_idle_sound)) {
					printf("Changed from state %d to state %d\n", enemy -> state, other_state);
					enemy -> state = other_state;
					state_changed = 1;
				}
			}
		}
	}

	if (state_changed) {
		printf("State changed\n");
		play_sound(enemy -> sounds[enemy -> state], 0);

		int new_frame_ind = 0;
		for (byte i = 0; i < enemy -> state; i++)
			new_frame_ind += enemy -> animation_seg_lengths[i];
		enemy -> animations.frame_ind = new_frame_ind;
	}

	switch (enemy -> state) {
		case Idle: // stay there, idle animation
			break;
		case Chasing: // bfs stuff
			update_path_if_needed(&enemy -> navigator, player.pos, player.jump.height);
			break;
		case Attacking: // attack animation + fighting
			break;
		case Retreating: // run away to some random vantage point
			break;
		case Dead: // do not move, dead single-sprite animation
			break;
	}
}

inlinable void set_message_pos(Message* const restrict message, const int x, const int y, const int w, const int h) {

	message -> pos = (SDL_Rect) {x, y, w, h};
}
