#version 460 core

#include "vertex_input_instanced.glsl"

layout (location = 0) out vec3 vFragPosWorldSpace;

layout (push_constant) uniform PushConstants
{
    mat4 lightSpaceMatrix;
};

void main()
{
    vFragPosWorldSpace = vec3(modelMat * vec4(position, 1.0));
    gl_Position = lightSpaceMatrix * vec4(vFragPosWorldSpace, 1.0);
}
