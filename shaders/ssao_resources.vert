#version 460 core

#include "vertex_input_instanced.glsl"

layout (location = 0) out vec3 vViewSpaceNormal;
layout (location = 1) out vec3 vViewSpacePos;

layout (set = 0, binding = 0) uniform CameraUBO
{
    mat4 view;
    mat4 projection;
    mat4 viewProj;
    vec4 cameraPos;
    vec4 cameraDir;
    float nearPlane;
    float farPlane;
};

void main()
{
    vViewSpaceNormal = mat3(view) * normalMat * normal;
    vViewSpacePos = vec3(view * modelMat * vec4(position, 1.0));

    gl_Position = viewProj * modelMat * vec4(position, 1.0);
}
