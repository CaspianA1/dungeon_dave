#version 330 core

out vec2 UV;

const vec2 screen_corners[4] = vec2[4](
	vec2(-1.0f, -1.0f), vec2(1.0f, -1.0f), vec2(-1.0f, 1.0f), vec2(1.0f, 1.0f)
);

void main(void) {
	vec2 screen_corner = screen_corners[gl_VertexID];

	gl_Position = vec4(screen_corner, 0.0f, 1.0f);
	UV = vec2(screen_corner.x, -screen_corner.y) * 0.5f + 0.5f;
}
