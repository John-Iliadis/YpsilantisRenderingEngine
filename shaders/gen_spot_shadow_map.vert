#version 460 core

#include "vertex_input_instanced.glsl"

layout (push_constant) uniform PushConstants
{
    mat4 lightSpaceMatrix;
};

void main()
{
    gl_Position = lightSpaceMatrix * modelMat * vec4(position, 1.0);
}
