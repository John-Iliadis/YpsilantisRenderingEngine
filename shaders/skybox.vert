#version 460 core

#include "camera_render_data.glsl"

layout (location = 0) in vec3 position;

layout (location = 0) out vec3 vTexCoords;

layout (set = 0, binding = 0) uniform CameraUBO { CameraRenderData cameraRenderData; };

void main()
{
    vTexCoords = position;
    vec4 pos = cameraRenderData.viewProj * vec4(position, 1.0);
    gl_Position = pos.xyww;
}
