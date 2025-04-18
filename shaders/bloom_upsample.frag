#version 460 core

layout (location = 0) in vec2 vTexCoords;

layout (location = 0) out vec4 outFragColor;

layout (push_constant) uniform PushConstants
{
    float filterRadius;
};

layout (set = 0, binding = 0) uniform sampler2D srcTexture;

const float tentFilter[9] = float[9] (
    1.0/16, 2.0/16, 1.0/16,
    2.0/16, 4.0/16, 2.0/16,
    1.0/16, 2.0/16, 1.0/16
);

void main()
{
    // Take 9 samples around current texel:
    // A - B - C
    // D - E - F
    // G - H - I
    vec2 offsets[9] = vec2[9] (
       vec2(-filterRadius, filterRadius), vec2(0.0, filterRadius), vec2(filterRadius, filterRadius),
       vec2(-filterRadius, 0.0), vec2(0.0, 0.0), vec2(filterRadius, 0.0),
       vec2(-filterRadius, -filterRadius), vec2(0.0, -filterRadius), vec2(filterRadius, -filterRadius)
    );

    outFragColor = vec4(0.0);
    for (int i = 0; i < 9; ++i)
    {
        outFragColor += texture(srcTexture, vTexCoords + offsets[i]) * tentFilter[i];
    }
}
