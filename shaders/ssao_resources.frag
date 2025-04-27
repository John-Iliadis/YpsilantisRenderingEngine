#version 460 core

#include "material.glsl"

layout (location = 0) in vec3 vViewSpaceNormal;
layout (location = 1) in vec3 vViewSpacePos;

layout (location = 0) out vec4 outNormal;
layout (location = 1) out vec4 outPos;

void main()
{
    outNormal = vec4(vViewSpaceNormal, 1.0);
    outPos = vec4(vViewSpacePos, 1.0);
}
