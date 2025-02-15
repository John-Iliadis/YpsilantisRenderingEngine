#version 460 core

layout (location = 0) in vec2 vTexCoords;

layout (location = 0) out float outFragColor;

layout (set = 0, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 projection;
    mat4 viewProj;
    vec4 cameraPos;
    vec4 cameraDir;
    float nearPlane;
    float farPlane;
};

layout (input_attachment_index = 0, set = 1, binding = 0) uniform subpassInput inputDepth;
layout (input_attachment_index = 1, set = 1, binding = 1) uniform subpassInput inputNormal;

void main()
{
    float depth = subpassLoad(inputDepth).r;
    vec4 normal = subpassLoad(inputNormal);

    outFragColor = 1.f;
}
