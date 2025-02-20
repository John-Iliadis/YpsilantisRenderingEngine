#version 460 core

#include "material.glsl"

#extension GL_EXT_nonuniform_qualifier : require

layout (location = 0) in vec2 vTexCoords;
layout (location = 1) in vec3 vNormal;

layout (location = 0) out vec4 outFragColor;

layout (push_constant) uniform PushConstants { int materialIndex; };

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

layout (set = 2, binding = 0) readonly buffer MaterialsSSBO { Material materials[]; };
layout (set = 2, binding = 1) uniform sampler2D textures[];

void main()
{
//    int baseColorTexIndex = materials[nonuniformEXT(materialIndex)].baseColorTexIndex;
//
//    vec4 base = materials[nonuniformEXT(materialIndex)].baseColorFactor;
//
//    vec4 baseCol = texture(textures[nonuniformEXT(baseColorTexIndex)], vTexCoords);
//
//    outFragColor = base * baseCol;
    outFragColor = vec4(vec3(0.5), 1.0);
}
