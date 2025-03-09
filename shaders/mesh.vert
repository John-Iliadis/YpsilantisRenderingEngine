#version 460 core

#include "vertex_input.glsl"

layout (location = 0) out vec3 vFragWorldPos;
layout (location = 1) out vec2 vTexCoords;
layout (location = 2) out mat3 vTBN;

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
    vFragWorldPos = vec3(modelMat * vec4(position, 1.0));
    vTexCoords = texCoords;

    vTBN = mat3(normalize(normalMat * tangent),
                normalize(normalMat * bitangent),
                normalize(normalMat * normal));

    gl_Position = viewProj * modelMat * vec4(position, 1.0);
}
