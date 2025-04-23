#version 460 core

layout (location = 0) in vec3 vFragPosWorldSpace;

layout (push_constant) uniform PushConstants
{
    layout (offset = 64) vec4 lightPos;
    float nearPlane;
    float farPlane;
};

void main()
{
    float lightDistance = length(vec3(vFragPosWorldSpace - lightPos.xyz));
    gl_FragDepth = lightDistance / farPlane;
}
