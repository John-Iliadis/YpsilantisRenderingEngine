#version 460 core

#define MAX_FRAGMENT_COUNT 128

#include "transparent_node.glsl"

layout (location = 0) in vec2 vTexCoords;
layout (location = 0) out vec4 outFragColor;

layout (input_attachment_index = 0,
        set = 0,
        binding = 0) uniform subpassInput opaqueColor;

layout (set = 1, binding = 0, r32ui) uniform uimage2D nodeIndexStorageTex;
layout (set = 1, binding = 1) buffer LinkedListSSBO { TransparentNode nodes[]; };
layout (set = 1, binding = 2) buffer NodeCounterSSBO
{
    uint nodeCount;
    uint maxNodeCount;
};

void main()
{
    TransparentNode fragments[MAX_FRAGMENT_COUNT];
    int count = 0;

    uint nodeIdx = imageLoad(nodeIndexStorageTex, ivec2(gl_FragCoord.xy)).r;

    // load the trasparent fragments
    while (nodeIdx != 0xffffffff && count < MAX_FRAGMENT_COUNT)
    {
        fragments[count] = nodes[nodeIdx];
        nodeIdx = fragments[count].next;
        ++count;
    }

    // sort the fragments
    for (uint i = 1; i < count; ++i)
    {
        TransparentNode insert = fragments[i];

        uint j = i;

        while (j > 0 && insert.depth > fragments[j - 1].depth)
        {
            fragments[j] = fragments[j - 1];
            --j;
        }

        fragments[j] = insert;
    }

    // blend all fragments
    outFragColor = subpassLoad(opaqueColor);
    for (uint i = 0; i < count; ++i)
    {
        outFragColor = mix(outFragColor, fragments[i].color, fragments[i].color.a);
    }
}
