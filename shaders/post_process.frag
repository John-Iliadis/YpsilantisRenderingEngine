#version 460 core

#include "tonemapping.glsl"

#define REINHARD 1
#define REINHARD_EXTENDED 2
#define NARKOWICZ_ACES 3
#define HILL_ACES 4

layout (location = 0) in vec2 vTexCoords;

layout (location = 0) out vec4 outFragColor;

layout (push_constant) uniform PushConstants
{
    uint hdrOn;
    float exposure;
    float maxWhite;
    uint tonemap;
    float bloomStrength;
};

layout (set = 0, binding = 0) uniform sampler2D srcTexture;
layout (set = 0, binding = 1) uniform sampler2D bloomTexture;

void main()
{
    vec3 color = texture(srcTexture, vTexCoords).rgb;
    vec3 bloom = texture(bloomTexture, vTexCoords).rgb;

    color += bloom * bloomStrength;

    if (hdrOn == 1)
    {
        color = applyExposure(color, exposure);

        if (tonemap == REINHARD) color = reinhard(color);
        else if (tonemap == REINHARD_EXTENDED) color = reinhardExtended(color, maxWhite);
        else if (tonemap == NARKOWICZ_ACES) color = narkowiczAces(color);
        else if (tonemap == HILL_ACES) color = hillAces(color);
    }

    outFragColor = vec4(color, 1.f);
}
