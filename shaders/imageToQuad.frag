#version 460 core

layout (location = 0) in vec2 vTexCoords;
layout (location = 0) out vec4 outFragColor;

layout (binding = 0) uniform sampler2D tex;

void main()
{
    outFragColor = texture(tex, vTexCoords);
}
