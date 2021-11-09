typedef struct {
	vec2 right_xz; // X and Z of right (Y is always 0)
	vec3 pos, dir; // The camera never moves from the origin, but `pos` here is more practical
	GLfloat fov, hori_angle, vert_angle, aspect_ratio;
	mat4 view_projection, model_view_projection; // Used the least, so last in struct
} Camera;

void init_camera(Camera* const camera, const vec3 init_pos) {
	memset(camera, 0, sizeof(Camera));
	memcpy(camera -> pos, init_pos, sizeof(vec3));
	camera -> fov = constants.init_fov;
	camera -> aspect_ratio = (GLfloat) SCR_W / SCR_H;
}

void update_camera(Camera* const camera) {
	static GLfloat last_time;
	static byte first_call = 1;

	if (first_call) {
		last_time = SDL_GetTicks() / 1000.0f;
		first_call = 0;
		return;
	}

	int mouse_dx, mouse_dy;
	SDL_GetRelativeMouseState(&mouse_dx, &mouse_dy);

	const GLfloat delta_time = (SDL_GetTicks() / 1000.0f) - last_time;
	camera -> hori_angle += constants.speeds.look * delta_time * -mouse_dx;
	camera -> vert_angle += constants.speeds.look * delta_time * -mouse_dy;

	if (camera -> vert_angle > constants.max_vert_angle) camera -> vert_angle = constants.max_vert_angle;
	else if (camera -> vert_angle < -constants.max_vert_angle) camera -> vert_angle = -constants.max_vert_angle;

	const GLfloat
		cos_vert = cosf(camera -> vert_angle),
		hori_angle_minus_half_pi = camera -> hori_angle - (GLfloat) M_PI_2,
		actual_speed = delta_time * constants.speeds.move;

	vec3 dir = {cos_vert * sinf(camera -> hori_angle), sinf(camera -> vert_angle), cos_vert * cosf(camera -> hori_angle)};
	memcpy(camera -> dir, dir, sizeof(vec3));

	camera -> right_xz[0] = sinf(hori_angle_minus_half_pi);
	camera -> right_xz[1] = cosf(hori_angle_minus_half_pi);

	vec3 right = {camera -> right_xz[0], 0.0f, camera -> right_xz[1]};
	if (keys[constants.movement_keys.forward]) glm_vec3_muladds(dir, actual_speed, camera -> pos);
	if (keys[constants.movement_keys.backward]) glm_vec3_muladds(dir, -actual_speed, camera -> pos);
	if (keys[constants.movement_keys.left]) glm_vec3_muladds(right, -actual_speed, camera -> pos);
	if (keys[constants.movement_keys.right]) glm_vec3_muladds(right, actual_speed, camera -> pos);

	//////////

	vec3 rel_origin, up;
	glm_vec3_add(camera -> pos, dir, rel_origin);
	glm_vec3_cross(right, camera -> dir, up);

	mat4 view, projection;
	glm_lookat(camera -> pos, rel_origin, up, view);

	glm_perspective(camera -> fov, camera -> aspect_ratio,
		constants.clip_dists.near, constants.clip_dists.far, projection);

	glm_mul(projection, view, camera -> view_projection); // For billboard shader
	glm_mul(camera -> view_projection, (mat4) GLM_MAT4_IDENTITY_INIT, camera -> model_view_projection); // For sector shader

	last_time = SDL_GetTicks() / 1000.0f;
}

void add_unculled_sectors_to_batch(Camera* const camera, Batch* const batch, const SectorList sector_list) {
	start_batch_adding(batch);

	vec4 frustum_planes[6];
	glm_frustum_planes(camera -> model_view_projection, frustum_planes);

	for (int i = 0; i < sector_list.length; i++) {
		const Sector sector = sector_list.data[i];
		const SectorArea area = sector.area;

		// In format of bottom left and top right
		vec3 aabb_corners[2] = {{area.origin[0], 0.0f, area.origin[1]}};
		glm_vec3_add(aabb_corners[0], (vec3) {area.size[0], area.height, area.size[1]}, aabb_corners[1]);

		// If sector in view
		if (glm_aabb_frustum(aabb_corners, frustum_planes)) {
			printf("Draw #%d\n", i + 1);
			add_to_batch(batch, NULL); // Feed sector vertices later on; store in sector, instead of vbo
		}
	}
}
