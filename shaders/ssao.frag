#version 460 core

#include "camera_render_data.glsl"

layout (location = 0) in vec2 vTexCoords;

layout (location = 0) out float outFragColor;

layout (set = 0, binding = 0) uniform CameraUBO { CameraRenderData cameraRenderData; };
layout (set = 1, binding = 0) uniform sampler2D depthTexture;
layout (set = 1, binding = 1) uniform sampler2D normalTexture;

void main()
{
    outFragColor = 1.f;
}
