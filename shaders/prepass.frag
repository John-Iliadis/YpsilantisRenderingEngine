#version 460 core

layout (location = 0) in vec3 vNormal;

layout (location = 0 ) out vec4 outNormal;

void main()
{
    outNormal = vec4(vNormal, 1.0);
}
