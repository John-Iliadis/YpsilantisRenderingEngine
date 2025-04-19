#version 460 core

#include "rendering_equations.glsl"

layout (location = 0) in vec3 vTexCoords;

layout (location = 0) out vec4 outFragColor;

layout (push_constant) uniform PushConstants
{
    float roughness;
};

layout (set = 0, binding = 1) uniform samplerCube envMap;

const uint sampleCount = 1024;

void main()
{
    vec3 normal = normalize(vTexCoords);
    vec3 R = normal;
    vec3 V = normal;

    vec3 prefilteredColor = vec3(0.0);
    float weight = 0.0;

    ivec2 faceSize = textureSize(envMap, 0);

    for (uint i = 0; i < sampleCount; ++i)
    {
        vec2 xi = hammersley(i, sampleCount);
        vec3 H = importanceSampleGGX(xi, normal, roughness);
        vec3 L = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = dot(normal, L);

        if(NdotL > 0.0)
        {
            float NdotH = max(dot(normal, H), 0.0);
            float HdotV = max(dot(H, V), 0.0);

            float D = D_GGX(NdotH, roughness);
            float pdf = D * NdotH / (4.0 * HdotV) + 0.0001;

            float resolution = faceSize.x;
            float saTexel  = 4.0 * PI / (6.0 * resolution * resolution);
            float saSample = 1.0 / (float(sampleCount) * pdf + 0.0001);

            float mipLevel = roughness == 0.0 ? 0.0 : 0.5 * log2(saSample / saTexel);

            prefilteredColor += textureLod(envMap, L, mipLevel).rgb * NdotL;
            weight += NdotL;
        }
    }

    prefilteredColor /= weight;

    outFragColor = vec4(prefilteredColor, 1.0);
}
