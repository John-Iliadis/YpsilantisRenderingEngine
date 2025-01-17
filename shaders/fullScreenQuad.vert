#version 460 core

layout (location = 0) out vec2 vTexCoords;

void main()
{
    vTexCoords = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(vTexCoords * 2.0 - 1.0, 0.0, 1.0);
}
