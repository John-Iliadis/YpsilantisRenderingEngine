#version 460 core

layout (location = 0) in vec2 vTexCoords;

layout (location = 0) out vec4 outFragColor;

layout (input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput inputTransparencyTex;

void main()
{
    outFragColor = subpassLoad(inputTransparencyTex);
}
