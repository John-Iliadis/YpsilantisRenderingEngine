#version 460 core

layout (location = 0) in vec2 vTexCoords;

layout (location = 0) out vec4 outNormal;

layout (push_constant) uniform PushConstants { uint sampleCount; };

layout (set = 0, binding = 0) uniform sampler2DMS normalTexture;
layout (set = 0, binding = 1) uniform sampler2DMS depthTexture;

void main()
{
    ivec2 texelCoords = ivec2(textureSize(normalTexture) * vTexCoords);

    uint sampleIndex = (uint(gl_FragCoord.x) + uint(gl_FragCoord.y)) % sampleCount;

    outNormal = texelFetch(normalTexture, texelCoords, int(sampleIndex));
    gl_FragDepth = texelFetch(depthTexture, texelCoords, int(sampleIndex)).r;
}
