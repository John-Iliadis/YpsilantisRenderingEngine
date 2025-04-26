#version 460 core

layout (location = 0) in vec2 vTexCoords;

layout (location = 0) out vec4 outFragColor;

layout (push_constant) uniform PushConstants
{
    float filterRadius;
    float width;
    float height;
};

layout (set = 0, binding = 0) uniform sampler2D srcTexture;

const vec2 offsets[9] = vec2[9] (
    vec2(-1.0,  1.0), vec2(0.0, 1.0), vec2(1.0, 1.0),
    vec2(-1.0,  0.0), vec2(0.0, 0.0), vec2(1.0, 0.0),
    vec2(-1.0,  -1.0), vec2(0.0, -1.0), vec2(1.0, -1.0)
);

void main()
{
    // Take 9 samples around current texel:
    // A - B - C
    // D - E - F
    // G - H - I
    vec4 A = texture(srcTexture, vTexCoords + filterRadius * offsets[0]);
    vec4 B = texture(srcTexture, vTexCoords + filterRadius * offsets[1]);
    vec4 C = texture(srcTexture, vTexCoords + filterRadius * offsets[2]);
    vec4 D = texture(srcTexture, vTexCoords + filterRadius * offsets[3]);
    vec4 E = texture(srcTexture, vTexCoords + filterRadius * offsets[4]);
    vec4 F = texture(srcTexture, vTexCoords + filterRadius * offsets[5]);
    vec4 G = texture(srcTexture, vTexCoords + filterRadius * offsets[6]);
    vec4 H = texture(srcTexture, vTexCoords + filterRadius * offsets[7]);
    vec4 I = texture(srcTexture, vTexCoords + filterRadius * offsets[8]);

    outFragColor = E * 4.0;
    outFragColor += (B + D + F + H) * 2.0;
    outFragColor += (A + C + G + I);
    outFragColor *= 1.0 / 16.0;
    outFragColor.a = 1.0;
}
