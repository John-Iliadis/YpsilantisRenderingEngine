#version 460 core

#include "material.glsl"

layout (location = 0) in vec3 vNormal;

layout (location = 0) out vec4 outNormal;

layout (set = 1, binding = 0) uniform MaterialsSSBO { Material material; };
layout (set = 1, binding = 1) uniform sampler2D baseColorTex;
layout (set = 1, binding = 2) uniform sampler2D metallicTex;
layout (set = 1, binding = 3) uniform sampler2D roughnessTex;
layout (set = 1, binding = 4) uniform sampler2D normalTex;
layout (set = 1, binding = 5) uniform sampler2D aoTex;
layout (set = 1, binding = 6) uniform sampler2D emissionTex;

void main()
{
    outNormal = vec4(vNormal, 1.0);
}
