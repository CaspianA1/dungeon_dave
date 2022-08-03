#version 400 core

noperspective in float sliding_UV_x;
noperspective in vec2 UV;
in vec3 pos_difference_from_light;

out vec3 color;

uniform float texture_transition_weight, specular_exponent;
uniform sampler2D logo_diffuse_sampler, palace_city_diffuse_sampler, palace_city_normal_map_sampler;

vec3 blinn_phong(const sampler2D diffuse_sampler, const vec2 custom_UV, const vec3 fragment_normal, const vec3 dir_to_light) {
	float diffuse = max(dot(fragment_normal, dir_to_light), 0.0f);

	vec3 halfway_dir = normalize(dir_to_light + dir_to_light); // Here, the view dir equals the dir to the light
	float cos_angle_of_incidence = max(dot(fragment_normal, halfway_dir), 0.0f);
	float specular = pow(cos_angle_of_incidence, specular_exponent);

	return texture(diffuse_sampler, custom_UV).rgb * (diffuse + specular);
}

void main(void) {
	vec2 sliding_UV = vec2(sliding_UV_x, UV.y);

	vec3
		dir_to_light = normalize(pos_difference_from_light),
		fragment_normal = texture(palace_city_normal_map_sampler, sliding_UV).rgb * 2.0f - 1.0f;

	color = mix(
		blinn_phong(logo_diffuse_sampler, UV, vec3(0.0f, 0.0f, 1.0f), dir_to_light),
		blinn_phong(palace_city_diffuse_sampler, sliding_UV, fragment_normal, dir_to_light),
		texture_transition_weight
	);
}
