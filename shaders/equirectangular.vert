#version 460 core

#include "cube_vertices.glsl"

layout (location = 0) out vec3 vPos;

void main()
{
    uint index = cubeIndices[gl_VertexIndex];
    vec3 pos = cubeVertices[index];

    vPos = pos;
    gl_Position = vec4(pos, 1.0);
}
