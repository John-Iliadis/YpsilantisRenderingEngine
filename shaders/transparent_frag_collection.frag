#version 460 core

// shadow mapping
// lighting
// ibl

#include "material.glsl"
#include "transparent_node.glsl"

layout (early_fragment_tests) in;

layout (location = 0) in vec2 vTexCoords;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec4 vColor;

layout (set = 1, binding = 0, r32ui) uniform coherent uimage2D nodeIndexStorageTex;

layout (set = 1, binding = 1) buffer LinkedListSSBO {
    TransparentNode nodes[];
};

layout (set = 1, binding = 2) buffer NodeCounterSSBO
{
    uint nodeCount;
    uint maxNodeCount;
};

layout (set = 2, binding = 0) uniform MaterialsUBO { Material material; };
layout (set = 2, binding = 1) uniform sampler2D baseColorTex;
layout (set = 2, binding = 2) uniform sampler2D metallicTex;
layout (set = 2, binding = 3) uniform sampler2D roughnessTex;
layout (set = 2, binding = 4) uniform sampler2D normalTex;
layout (set = 2, binding = 5) uniform sampler2D aoTex;
layout (set = 2, binding = 6) uniform sampler2D emissionTex;

void main()
{
    uint nodeIndex = atomicAdd(nodeCount, 1);

    if (nodeIndex >= maxNodeCount)
        discard;

    vec4 baseColorFactor = material.baseColorFactor;
    vec4 baseColorSample = texture(baseColorTex, vTexCoords * material.tiling);

    vec4 color = vColor * baseColorFactor * baseColorSample;
    uint previousNodeIndex = imageAtomicExchange(nodeIndexStorageTex, ivec2(gl_FragCoord.xy), nodeIndex);

    nodes[nodeIndex] = TransparentNode(color, gl_FragCoord.z, previousNodeIndex);
}
