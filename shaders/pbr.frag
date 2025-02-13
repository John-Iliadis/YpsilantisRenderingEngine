#version 460 core

#extension GL_EXT_nonuniform_qualifier : require

//NonUniformEXT(textureIndex)

#include "camera_render_data.glsl"
#include "material.glsl"

layout (location = 0) in vec2 vTexCoords;
layout (location = 1) in vec3 vNormal;

layout (location = 0) out vec4 outFragColor;

layout (push_constant) uniform PushConstants { int materialIndex; };

layout (set = 0, binding = 0) uniform CameraUBO { CameraRenderData cameraRenderData; };
layout (set = 1, binding = 0) uniform sampler2D ssaoTexture;
layout (set = 2, binding = 0) uniform MaterialsUBO { Material materials[]; };
layout (set = 2, binding = 1) uniform sampler2D textures[];

void main()
{
    outFragColor = vec4(0.5, 0.5, 0.5, 1.0);
}
