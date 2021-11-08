typedef struct {
	// Technically, the camera stays at {0, 0, 0}, but having the `pos` member is more practical
	GLfloat pos[3], dir[3], right[3], up[3], fov, hori_angle, vert_angle, aspect_ratio;
} Camera;

// An update_camera fn would be nice

void init_camera(Camera* const camera, const GLfloat init_pos[3], const plane_type_t map_size[2]) {
	(void) map_size;

	memset(camera, 0, sizeof(Camera));
	memcpy(camera -> pos, init_pos, sizeof(GLfloat) * 3);
	camera -> fov = constants.init_fov;
	camera -> aspect_ratio = (GLfloat) SCR_W / SCR_H;
}

void move_camera(Camera* const camera) {
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

	// half_pi = fully up, or -half_pi = fully down
	if (camera -> vert_angle > constants.numbers.half_pi) camera -> vert_angle = constants.numbers.half_pi;
	else if (camera -> vert_angle < -constants.numbers.half_pi) camera -> vert_angle = -constants.numbers.half_pi;

	const GLfloat
		cos_vert = cosf(camera -> vert_angle),
		hori_angle_minus_half_pi = camera -> hori_angle - constants.numbers.half_pi,
		actual_speed = delta_time * constants.speeds.move;

	vec3 dir = {cos_vert * sinf(camera -> hori_angle), sinf(camera -> vert_angle), cos_vert * cosf(camera -> hori_angle)};
	memcpy(camera -> dir, dir, sizeof(vec3));

	vec3 right = {sinf(hori_angle_minus_half_pi), 0.0f, cosf(hori_angle_minus_half_pi)};
	memcpy(camera -> right, right, sizeof(vec3));

	if (keys[constants.movement_keys.forward]) glm_vec3_muladds(dir, actual_speed, camera -> pos);
	if (keys[constants.movement_keys.backward]) glm_vec3_muladds(dir, -actual_speed, camera -> pos);
	if (keys[constants.movement_keys.left]) glm_vec3_muladds(right, -actual_speed, camera -> pos);
	if (keys[constants.movement_keys.right]) glm_vec3_muladds(right, actual_speed, camera -> pos);

	vec3 pos_plus_dir, up;
	glm_vec3_add(camera -> pos, camera -> dir, pos_plus_dir);
	glm_vec3_cross(right, camera -> dir, up);

	// Configure matrices
	puts("Configure matrices");

	last_time = SDL_GetTicks() / 1000.0f;
}

// Later on, this may offer the caller different matrices as well, if needed
void get_matrices_from_camera(const Camera* const camera, mat4 model_view_projection) {
	GLfloat* const pos = (GLfloat*) camera -> pos;
	mat4 projection, view, model_view;
	vec3 rel_origin;

	glm_vec3_add(pos, (GLfloat*) camera -> dir, rel_origin);
	glm_lookat(pos, rel_origin, (GLfloat*) camera -> up, view);
	glm_mul(view, (mat4) GLM_MAT4_IDENTITY_INIT, model_view);

	glm_perspective(camera -> fov, camera -> aspect_ratio, constants.clip_dists.near, constants.clip_dists.far, projection);
	glm_mul(projection, model_view, model_view_projection);
}

void add_unculled_sectors_to_batch(const Camera* const camera, Batch* const batch, const SectorList sector_list) {
	start_batch_adding(batch);

	mat4 model_view_projection;
	get_matrices_from_camera(camera, model_view_projection);

	vec4 frustum_planes[6];
	glm_frustum_planes(model_view_projection, frustum_planes);

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
