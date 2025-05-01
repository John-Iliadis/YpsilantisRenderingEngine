#version 460 core

#include "bloom.glsl"

layout (location = 0) in vec2 vTexCoords;

layout (location = 0) out vec4 outFragColor;

layout (push_constant) uniform PushConstants
{
    vec2 texelSize;
    float threshold;
};

layout (set = 0, binding = 0) uniform sampler2D tex;

void main()
{
    vec3 color = downsample13(tex, vTexCoords, texelSize).rgb;

    float brighness = max(color.r, max(color.g, color.b));
    color *= float(brighness > threshold);

    outFragColor = vec4(color, 1.0);
}
