#version 460 core

#include "vertex_input.glsl"

layout (location = 0) out vec3 vFragWorldPos;
layout (location = 1) out vec2 vTexCoords;
layout (location = 2) out mat3 vTBN;

layout (push_constant) uniform PushConstants
{
    layout (offset = 64) mat4 model;
};

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
    vFragWorldPos = vec3(model * vec4(position, 1.0));
    vTexCoords = texCoords;

    mat3 normalMat = inverse(transpose(mat3(model)));

    vTBN = mat3(normalize(normalMat * tangent),
                normalize(normalMat * bitangent),
                normalize(normalMat * normal));

    gl_Position = viewProj * model * vec4(position, 1.0);
}
