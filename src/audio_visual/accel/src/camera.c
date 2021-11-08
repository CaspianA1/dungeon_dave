typedef struct {
	// Technically, the camera stays at {0, 0, 0}, but having the `pos` member is more practical
	GLfloat pos[3], dir[3], right[3], up[3], move_speed, look_speed, fov, hori_angle, vert_angle, aspect_ratio;
} Camera;


/* Max world size = 255 by 255 by 255 (with top left corner of block as origin)
So, max look distance in world = sqrt(255 * 255 + 255 * 255 + 255 * 255), which equals 441.6729559300637 */

// An update_camera fn would be nice

void init_camera(Camera* const camera, const plane_type_t map_size[2]) {
	(void) camera;
	(void) map_size;
}

/*
void update_camera(Camera* const camera) {
	(void) camera;

	static byte first_call = 1;
	if (first_call) {
		first_call = 0;
		return;
	}
}
*/

// Later on, this may offer the caller different matrices as well, if needed
void get_matrices_from_camera(const Camera* const camera, mat4 model_view_projection) {
	GLfloat* const pos = (GLfloat*) camera -> pos;
	mat4 projection, view, model_view;
	vec3 rel_origin;

	glm_vec3_add(pos, (GLfloat*) camera -> dir, rel_origin);
	glm_lookat(pos, rel_origin, (GLfloat*) camera -> up, view);
	glm_mul(view, (mat4) GLM_MAT4_IDENTITY_INIT, model_view);

	glm_perspective(camera -> fov, camera -> aspect_ratio, clip_dists.near, clip_dists.far, projection);
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
