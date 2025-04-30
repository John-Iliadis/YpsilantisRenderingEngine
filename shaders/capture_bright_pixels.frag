#version 460 core

#include "bloom.glsl"

layout (location = 0) in vec2 vTexCoords;

layout (location = 0) out vec4 outFragColor;

layout (push_constant) uniform PushConstants
{
    float brightnessThreshold;
    float knee;
};

layout (set = 0, binding = 0) uniform sampler2D tex;

void main()
{
    vec2 texelSize = 1.0 / vec2(textureSize(tex, 0));
    vec3 color = downsample13(tex, vTexCoords, texelSize).rgb;
    vec3 curve = vec3(brightnessThreshold - knee, knee * 2.0, 0.25 / knee);
    color = quadraticThreshold(color, brightnessThreshold, curve);
    outFragColor = vec4(color, 1.0);
}
