#version 400 core // TODO: figure out how to change the version to 330

// TODO: can remove the `invocations` part?
layout(triangles, invocations = 5) in;
layout(triangle_strip, max_vertices = 3) out;
	
uniform mat4 light_space_matrices[16];
	
void main(void) {
	for (int i = 0; i < 3; i++) {
		gl_Position = light_space_matrices[gl_InvocationID] * gl_in[i].gl_Position;
		gl_Layer = gl_InvocationID;
		EmitVertex();
	}
	EndPrimitive();
}  
