#version 460 core

#include "vertex_input.glsl"
#include "camera_render_data.glsl"

layout (location = 0) out vec3 vNormal;

layout (set = 0, binding = 0) uniform CameraUBO { CameraRenderData cameraRenderData; };

void main()
{
    vNormal = normalMat * normal;
    gl_Position = cameraRenderData.viewProj * modelMat * vec4(position, 1.0);
}
