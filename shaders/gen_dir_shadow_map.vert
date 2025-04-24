#version 460 core

#include "vertex_input.glsl"

layout (push_constant) uniform PushConstants
{
    mat4 viewProj;
};

void main()
{
    gl_Position = viewProj * modelMat * vec4(position, 1.0);
}
