#version 460 core

#include "cube_vertices.glsl"

layout (location = 0) out vec3 vTexCoords;

layout (push_constant) uniform PushConstants
{
    mat4 proj;
};

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
    uint index = cubeIndices[gl_VertexIndex];
    vec3 position = cubeVertices[index];

    vTexCoords = position;

    gl_Position = (proj * mat4(mat3(view)) * vec4(position, 1.0)).xyww;
}
