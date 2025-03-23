#version 460 core

layout (location = 0) in vec2 vTexCoords;

layout (location = 0) out float outFragValue;

layout (push_constant) uniform PushConstants
{
    vec2 texelSize;
};

layout (set = 0, binding = 0) uniform sampler2D ssaoTexture;

void main()
{
    outFragValue = 0.0;

    for (int x = -2; x <= 2; ++x)
    {
        for (int y = -2; y <= 2; ++y)
        {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            outFragValue += texture(ssaoTexture, vTexCoords + offset).r;
        }
    }

    outFragValue /= 25;
}
