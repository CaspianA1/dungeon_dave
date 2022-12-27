#version 400 core

#include "common/shared_params.glsl"

in vec3 cube_edge;

out vec3 color;

uniform bool map_cube_to_sphere;
uniform samplerCube skybox_sampler;

/* Sources/ideas on going from equiangular to equirectangular:
https://www.reddit.com/r/Unity3D/comments/6vfdpc/how_does_one_create_a_skybox_cubemap_from_scratch/
https://vrkiwi.org/dev-blog/35-how-to-make-a-skybox-or-two/
https://www.reddit.com/r/gamedev/comments/crxcu8/how_does_one_make_a_seamless_cube_shaped_skybox/
https://plumetutorials.wordpress.com/2013/10/09/3d-tutorial-making-a-skybox/
https://blog.google/products/google-ar-vr/bringing-pixels-front-and-center-vr-video/

The thumbnail to this: https://onlinelibrary.wiley.com/doi/10.1111/cgf.13843
is this: https://onlinelibrary.wiley.com/cms/asset/a86e9106-591f-4d4c-9edb-2adde15de666/cgf13843-fig-0001-m.jpg
Interesting image!

Note: radial projection may be the key */
void main(void) {
	vec3 remapped_cube_edge = cube_edge;

	if (map_cube_to_sphere) {
		/* This cube -> sphere mapping comes from here:
		http://mathproofs.blogspot.com/2005/07/mapping-cube-to-sphere.html.
		TODO: make this mapping more robust, and more evenly distorted throughout
		(not just at the edges).

		Also note that the normalization of the sphere edge position doesn't change anything. */

		vec3 squared = remapped_cube_edge * remapped_cube_edge;
		vec3 left = squared.yxx, right = squared.zzy;
		vec3 products = left * right, sums = left + right;

		const float one_third = 1.0f / 3.0f;
		remapped_cube_edge *= inversesqrt(1.0f + (products * one_third) - (sums * 0.5f));
	}

	color = texture(skybox_sampler, remapped_cube_edge).rgb * light_color;
}
