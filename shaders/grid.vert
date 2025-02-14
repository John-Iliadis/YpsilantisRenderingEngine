#version 460 core

layout (location = 0) out vec2 vTexCoords;
layout (location = 1) out vec2 vCameraPos;

const vec4 positions[4] = vec4[4](
    vec4(-1.0, 0.0, -1.0, 1.0),
    vec4( 1.0, 0.0, -1.0, 1.0),
    vec4( 1.0, 0.0,  1.0, 1.0),
    vec4(-1.0, 0.0,  1.0, 1.0)
);

const int indices[6] = int[6](0, 1, 2, 2, 3, 0);
const float gridSize = 100.0;

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
    int index = indices[gl_VertexIndex];
    vec4 position = vec4(positions[index].xyz * gridSize, 1.f);

    vCameraPos = cameraPos.xz;

    position.xz += vCameraPos.xy;

    vTexCoords = position.xz;

    gl_Position = viewProj * position;
}
