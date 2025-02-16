#version 460 core

layout (location = 0) in vec3 position;

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
    vTexCoords = position;
    vec4 pos = view * vec4(position, 1.0);
    gl_Position = vec4(pos.xyz, pos.z);
}
