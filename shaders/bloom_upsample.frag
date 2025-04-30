#version 460 core

#include "bloom.glsl"

layout (location = 0) in vec2 vTexCoords;

layout (location = 0) out vec4 outFragColor;

layout (push_constant) uniform PushConstants
{
    float filterRadius;
    float width;
    float height;
};

layout (set = 0, binding = 0) uniform sampler2D srcTexture;

void main()
{
    vec2 texelSize = 1.0 / vec2(width, height);
    vec3 result = upsample9(srcTexture, vTexCoords, texelSize, filterRadius);
    outFragColor = vec4(result, 1.0);
}
