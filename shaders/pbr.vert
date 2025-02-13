#version 460 core

#include "camera_render_data.glsl"
#include "vertex_input.glsl"

layout (location = 0) out vec2 vTexCoords;
layout (location = 1) out vec3 vNormal;

layout (set = 0, binding = 0) uniform CameraUBO { CameraRenderData cameraRenderData; };

void main()
{
    vTexCoords = texCoords;
    vNormal = normalMat * normal;
    gl_Position = cameraRenderData.viewProj * modelMat * vec4(position, 1.0);
}
