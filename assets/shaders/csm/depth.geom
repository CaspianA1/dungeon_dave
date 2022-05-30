#version 400 core // TODO: figure out how to change the version to 330

// TODO: can I remove the `invocations` part?
layout(triangles, invocations = 5) in;
layout(triangle_strip, max_vertices = 3) out;
	
// TODO: initialize this uniform
uniform mat4 light_space_matrices[16];
	
void main(void) {
	for (int i = 0; i < 3; i++) { // TODO: make 3 a constant or uniform
		gl_Position = light_space_matrices[gl_InvocationID] * gl_in[i].gl_Position;
		gl_Layer = gl_InvocationID; // This sets the cascade layer for the render invocation
		EmitVertex();
	}
	EndPrimitive();
}  
