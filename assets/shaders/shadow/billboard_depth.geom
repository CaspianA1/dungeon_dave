#version 400 core

#include "depth.geom"

DEFINE_GEOMETRY_STAGE_OF_DEPTH_SHADER(
	flat in vec3 vertex_UV[]; out vec3 UV; , UV = vertex_UV[i];
)
