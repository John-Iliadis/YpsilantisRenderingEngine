#version 460 core

#include "vertex_input.glsl"

layout (location = 0) out vec3 vViewSpacePos;
layout (location = 1) out vec3 vViewSpaceNormal;

layout (set = 0, binding = 0) uniform CameraUBO {
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
    vViewSpacePos = vec3(view * modelMat * vec4(position, 1.0));
    vViewSpaceNormal = mat3(view) * normalMat * normal;

    gl_Position = viewProj * modelMat * vec4(position, 1.0);
}
