/*
Max world size = 255 by 255 by 255 (with top left corner of block as origin).
So, max look distance in world = sqrt(255 * 255 + 255 * 255 + 255 * 255),
which equals 441.6729559300637
*/

static const struct {

    const struct {
        const GLfloat near, far;
    } clip_dists;

    const struct {
        const GLfloat move, look;
    } speeds;

    const struct {
        const GLfloat half_pi;
    } numbers;

} constants = {
    .clip_dists = {0.1f, 441.6729559300637f},
    .speeds = {3.0f, 0.08f},
    .numbers = {(GLfloat) M_PI / 2.0f}
};
