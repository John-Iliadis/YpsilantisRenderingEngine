#version 460 core

#include "material.glsl"

layout (early_fragment_tests) in;

layout (location = 0) in vec2 vTexCoords;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec4 vColor;

layout (location = 0) out vec4 outFragColor;

layout (set = 0, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 projection;
    mat4 viewProj;
    vec4 cameraPos;
    vec4 cameraDir;
    float nearPlane;
    float farPlane;
};

layout (set = 1, binding = 0) uniform sampler2D ssaoTexture;

layout (set = 2, binding = 0) uniform MaterialsUBO { Material material; };
layout (set = 2, binding = 1) uniform sampler2D baseColorTex;
layout (set = 2, binding = 2) uniform sampler2D metallicTex;
layout (set = 2, binding = 3) uniform sampler2D roughnessTex;
layout (set = 2, binding = 4) uniform sampler2D normalTex;
layout (set = 2, binding = 5) uniform sampler2D aoTex;
layout (set = 2, binding = 6) uniform sampler2D emissionTex;

void main()
{
    vec4 color = vColor;
    vec4 baseColorFactor = material.baseColorFactor;
    vec4 baseColorSample = texture(baseColorTex, vTexCoords * material.tiling);

    outFragColor = color * baseColorFactor * baseColorSample;
}
