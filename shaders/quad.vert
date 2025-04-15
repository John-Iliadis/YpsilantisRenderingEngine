#version 460 core

#include "quad_vertices.glsl"

void main()
{
    uint index = quadIndices[gl_VertexIndex];
    vec3 pos = quadVertices[index];
    gl_Position = vec4(pos, 1.0);
}
