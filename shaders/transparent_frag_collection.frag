#version 460 core

// shadow mapping
// ibl

#include "transparent_node.glsl"
#include "material.glsl"
#include "lights.glsl"
#include "rendering_equations.glsl"

layout (early_fragment_tests) in;

layout (location = 0) in vec3 vFragWorldPos;
layout (location = 1) in vec2 vTexCoords;
layout (location = 2) in mat3 vTBN;

layout (push_constant) uniform PushConstants
{
    uint dirLightCount;
    uint pointLightCount;
    uint spotLightCount;
};

layout (set = 0, binding = 0) uniform CameraUBO
{
    mat4 view;
    mat4 projection;
    mat4 viewProj;
    vec4 cameraPos;
    vec4 cameraDir;
    float nearPlane;
    float farPlane;
};

layout (set = 1, binding = 0, r32ui) uniform coherent uimage2D nodeIndexStorageTex;

layout (set = 1, binding = 1) buffer LinkedListSSBO {
    TransparentNode nodes[];
};

layout (set = 1, binding = 2) buffer NodeCounterSSBO
{
    uint nodeCount;
    uint maxNodeCount;
};

layout (set = 2, binding = 0) buffer readonly DirLightsSSBO { DirectionalLight dirLights[]; };
layout (set = 2, binding = 1) buffer readonly PointLightsSSBO { PointLight pointLights[]; };
layout (set = 2, binding = 2) buffer readonly SpotLightSSBO { SpotLight spotLights[]; };

layout (set = 3, binding = 0) uniform MaterialsUBO { Material material; };
layout (set = 3, binding = 1) uniform sampler2D baseColorTex;
layout (set = 3, binding = 2) uniform sampler2D metallicTex;
layout (set = 3, binding = 3) uniform sampler2D roughnessTex;
layout (set = 3, binding = 4) uniform sampler2D normalTex;
layout (set = 3, binding = 5) uniform sampler2D aoTex;
layout (set = 3, binding = 6) uniform sampler2D emissionTex;

void main()
{
    uint nodeIndex = atomicAdd(nodeCount, 1);

    if (nodeIndex < maxNodeCount)
    {
        vec4 baseColorFactor = material.baseColorFactor;
        vec4 baseColorSample = texture(baseColorTex, vTexCoords * material.tiling);

        vec4 color = baseColorFactor * baseColorSample;
        uint previousNodeIndex = imageAtomicExchange(nodeIndexStorageTex, ivec2(gl_FragCoord.xy), nodeIndex);

        nodes[nodeIndex] = TransparentNode(color, gl_FragCoord.z, previousNodeIndex);
    }
}
