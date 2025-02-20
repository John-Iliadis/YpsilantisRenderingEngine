#version 460 core

#include "material.glsl"

#extension GL_EXT_nonuniform_qualifier : require

layout (location = 0) in vec3 vNormal;

layout (location = 0) out vec4 outNormal;

layout (push_constant) uniform PushConstants { int materialIndex; };

layout (set = 1, binding = 0) readonly buffer MaterialsSSBO { Material materials[]; };
layout (set = 1, binding = 1) uniform sampler2D textures[];

void main()
{
    outNormal = vec4(vNormal, 1.0);
}
