#version 460 core

#define MAX_MATERIALS 32
#define MAX_TEXTURES 128

#include "material.glsl"

layout (location = 0) in vec3 vNormal;

layout (location = 0) out vec4 outNormal;

layout (push_constant) uniform PushConstants { int materialIndex; };

layout (set = 1, binding = 0) uniform MaterialsUBO { Material materials[MAX_MATERIALS]; };
layout (set = 1, binding = 1) uniform sampler2D textures[MAX_TEXTURES];

void main()
{
    outNormal = vec4(vNormal, 1.0);
}
