#version 460 core

#include "rendering_equations.glsl"

layout (location = 0) in vec2 vTexCoords;

layout (location = 0) out vec4 outFragColor;

const uint sampleCount = 1024;

void main()
{
    vec2 intergratedBDRF = integrateBRDF(vTexCoords.x, vTexCoords.y, sampleCount);
    outFragColor = vec4(intergratedBDRF, 0.0, 1.0);
}
