/*
Max world size = 255 by 255 by 255 (with top left corner of block as origin).
So, max look distance in world = sqrt(255 * 255 + 255 * 255 + 255 * 255),
which equals 441.6729559300637
*/

static const struct {
    // Angles are in radians
    const GLfloat init_fov, max_vert_angle;

    const struct {
        const GLfloat near, far;
    } clip_dists;

    const struct {
        const GLfloat move, look;
    } speeds;

    const struct {
        const SDL_Scancode forward, backward, left, right;
    } movement_keys;

} constants = {
    .init_fov = (GLfloat) M_PI_2, // 90 degrees
    .max_vert_angle = (GLfloat) M_PI_2,
    .clip_dists = {0.1f, 441.6729559300637f},
    .speeds = {3.0f, 0.08f},
    .movement_keys = {SDL_SCANCODE_W, SDL_SCANCODE_S, SDL_SCANCODE_A, SDL_SCANCODE_D}
};