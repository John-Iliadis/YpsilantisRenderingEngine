#version 460 core

layout (early_fragment_tests) in;

layout (location = 0) in vec3 vTexCoords;

layout (location = 0) out vec4 outFragColor;

layout (set = 1, binding = 0) uniform samplerCube cubemap;

void main()
{
    outFragColor = texture(cubemap, vTexCoords);
}
