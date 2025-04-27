#version 460 core

layout (location = 0) in vec2 vTexCoords;

layout (location = 0) out float outFragValue;

layout (push_constant) uniform PushConstants
{
    vec2 texelSize;
    vec2 direction;
};

layout (set = 0, binding = 0) uniform sampler2D tex;

const float weight[5] = float[5] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main()
{
    outFragValue = texture(tex, vTexCoords).r * weight[0];

    for (uint i = 1; i < 5; ++i)
    {
        outFragValue += texture(tex, vTexCoords + float(i) * texelSize * direction).r * weight[i];
        outFragValue += texture(tex, vTexCoords - float(i) * texelSize * direction).r * weight[i];
    }
}
