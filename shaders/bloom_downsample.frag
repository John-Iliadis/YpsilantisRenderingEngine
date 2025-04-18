#version 460 core

layout (location = 0) in vec2 vTexCoords;

layout (location = 0) out vec4 outFragColor;

layout (push_constant) uniform PushConstants
{
    uint width;
    uint height;
    uint mipLevel;
};

layout (set = 0, binding = 0) uniform sampler2D srcTexture;

const vec2 offsets[13] = vec2[13] (
    vec2(-2.0,  2.0), vec2( 0.0,  2.0), vec2( 2.0,  2.0),
    vec2(-1.0,  1.0), vec2( 1.0,  1.0),
    vec2(-2.0,  0.0), vec2( 0.0,  0.0), vec2( 2.0,  0.0),
    vec2(-1.0, -1.0), vec2( 1.0, -1.0),
    vec2(-2.0, -2.0), vec2( 0.0, -2.0), vec2( 2.0, -2.0)
);

void main()
{
    vec2 srcTexelSize = 1.0 / vec2(width, height);

    // Take 13 samples around current texel:
    // A - B - C
    // - D - E -
    // F - G - H
    // - I - J -
    // K - L - M
    // ---------
    vec4 A = texture(srcTexture, vTexCoords + srcTexelSize * offsets[0]);
    vec4 B = texture(srcTexture, vTexCoords + srcTexelSize * offsets[1]);
    vec4 C = texture(srcTexture, vTexCoords + srcTexelSize * offsets[2]);
    vec4 D = texture(srcTexture, vTexCoords + srcTexelSize * offsets[3]);
    vec4 E = texture(srcTexture, vTexCoords + srcTexelSize * offsets[4]);
    vec4 F = texture(srcTexture, vTexCoords + srcTexelSize * offsets[5]);
    vec4 G = texture(srcTexture, vTexCoords + srcTexelSize * offsets[6]);
    vec4 H = texture(srcTexture, vTexCoords + srcTexelSize * offsets[7]);
    vec4 I = texture(srcTexture, vTexCoords + srcTexelSize * offsets[8]);
    vec4 J = texture(srcTexture, vTexCoords + srcTexelSize * offsets[9]);
    vec4 K = texture(srcTexture, vTexCoords + srcTexelSize * offsets[10]);
    vec4 L = texture(srcTexture, vTexCoords + srcTexelSize * offsets[11]);
    vec4 M = texture(srcTexture, vTexCoords + srcTexelSize * offsets[12]);


    // Apply weighted distribution:
    // 0.5 + 0.125 + 0.125 + 0.125 + 0.125 = 1
    vec2 div = 0.25 * vec2(0.5, 0.125);
    vec4 center = (D + E + I + J) * div.x;
    vec4 topLeft = (A + B + F + G) * div.y;
    vec4 topRight = (B + C + G + H) * div.y;
    vec4 bottomLeft = (F + G + K + L) * div.y;
    vec4 bottomRight = (G + H + L + M) * div.y;

    outFragColor = center + topLeft + topRight + bottomLeft + bottomRight;
}
