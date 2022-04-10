#ifndef NORMAL_MAP_GENERATION_H
#define NORMAL_MAP_GENERATION_H

/* Excluded: limit_int_to_domain, read_surface_pixel, edge_checked_read_surface_pixel,
sobel_sample, generate_normal_map, compute_1D_gaussian_kernel, do_separable_gaussian_blur_pass */

GLuint init_normal_map_set_from_texture_set(const GLuint texture_set);

#endif
