#version 460 core

#include "material.glsl"

#define MAX_MATERIALS 32
#define MAX_TEXTURES 128

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

layout (set = 2, binding = 0) readonly buffer MaterialsSSBO { Material materials[MAX_MATERIALS]; };
layout (set = 2, binding = 1) uniform sampler2D textures[MAX_TEXTURES];

void main()
{
    outFragColor = vec4(0.5);
}
