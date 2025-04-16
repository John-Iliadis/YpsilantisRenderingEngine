#version 460 core

#include "rendering_equations.glsl"

layout (location = 0) in vec3 vTexCoords;

layout (location = 0) out vec4 outFragColor;

layout (push_constant) uniform PushConstants
{
    float roughness;
};

layout (set = 0, binding = 1) uniform samplerCube envMap;

// can increase this to prevent dots
const uint sampleCount = 1024;

void main()
{
    vec3 normal = normalize(vTexCoords);
    vec3 R = normal;
    vec3 V = normal;

    vec3 prefilteredColor = vec3(0.0);
    float weight = 0.0;

    for (uint i = 0; i < sampleCount; ++i)
    {
        vec2 xi = hammersley(i, sampleCount);
        vec3 H = importanceSampleGGX(xi, normal, roughness);
        vec3 L = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(dot(normal, L), 0.0);

        prefilteredColor += texture(envMap, L).rgb * NdotL;
        weight += NdotL;
    }

    prefilteredColor /= weight;

    outFragColor = vec4(prefilteredColor, 1.0);
}
