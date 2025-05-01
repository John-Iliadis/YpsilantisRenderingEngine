#version 460 core

#include "bloom.glsl"

layout (location = 0) in vec2 vTexCoords;

layout (location = 0) out vec4 outFragColor;

layout (push_constant) uniform PushConstants
{
    vec2 texelSize;
    uint mipLevel;
};

layout (set = 0, binding = 0) uniform sampler2D srcTexture;

void main()
{
    vec3 result = downsample13(srcTexture, vTexCoords, texelSize);
    outFragColor = vec4(result, 1.0);
}
