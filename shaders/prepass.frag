#version 460 core

#include "material.glsl"

layout (location = 0) in vec3 vNormal;

layout (location = 0) out vec4 outNormal;

layout (set = 1, binding = 0) uniform MaterialsUBO { Material materials[]; };
layout (set = 1, binding = 1) uniform sampler2D textures[];

void main()
{
    outNormal = vec4(vNormal, 1.0);
}
