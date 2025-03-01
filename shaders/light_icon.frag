#version 460

layout (location = 0) in vec2 vTexCoords;

layout (location = 0) out vec4 outFragColor;

layout (set = 1, binding = 0) uniform sampler2D lightIcon;
layout (set = 1, binding = 1) uniform sampler2D depthTexture;

void main()
{
    vec4 color = texture(lightIcon, vTexCoords);
    float depth = texture(depthTexture, gl_FragCoord.xy / vec2(textureSize(depthTexture, 0))).r;

    outFragColor = color * float(gl_FragCoord.z <= depth) +
                   color * vec4(vec3(0.8), color.a) * float(gl_FragCoord.z > depth);
}
