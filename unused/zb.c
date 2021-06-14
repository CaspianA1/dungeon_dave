void some_fn(void) {
	memset(current_level.z_buffers_filled, 0, current_level.max_point_height);

	for (int i = 0; i < point_height; i++) {
		if (!current_level.z_buffers_filled[i]) {
			current_level.z_buffers[i][screen_x] = correct_dist;
		}
	}

	for (byte i = 0; i < current_level.max_point_height; i++) {
		double* curr_z_buffer = screen.z_buffers[i];
		if (curr_z_buffer[screen_row] < billboard.dist) continue;
	}
}