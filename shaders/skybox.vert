#version 460 core

#include "cube_data.glsl"

layout (location = 0) out vec3 vTexCoords;

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

    mat4 viewNoTranslation = mat4(mat3(view));

    vec4 viewProjPos = projection * viewNoTranslation * vec4(position, 1.0);
    gl_Position = viewProjPos.xyww;
}
