#version 460 core

#include "camera_render_data.glsl"

layout (location = 0) out vec2 vTexCoords;
layout (location = 1) out vec3 vCameraPos;

const vec4 positions[4] = vec4[4](
    vec4(-1.0, 0.0, -1.0, 1.0),
    vec4( 1.0, 0.0, -1.0, 1.0),
    vec4( 1.0, 0.0,  1.0, 1.0),
    vec4(-1.0, 0.0,  1.0, 1.0)
);

const int indices[6] = int[6](0, 1, 2, 2, 3, 0);

layout (set = 0, binding = 0) uniform CameraUBO { CameraRenderData cameraRenderData; };

void main()
{
    int index = indices[gl_VertexIndex];
    vec4 position = positions[index];

    mat4 inverseView = inverse(cameraRenderData.view);

    position.x = cameraRenderData.position.x;
    position.y = cameraRenderData.position.y;

    vTexCoords = position.xz;
    vCameraPos = cameraRenderData.position.xyz;

    gl_Position = cameraRenderData.viewProj * position;
}
