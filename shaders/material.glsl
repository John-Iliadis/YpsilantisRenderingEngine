
const uint AlphaModeOpaque = 0;
const uint AlphaModeMask = 1;
const uint AlphaModeBlend = 2;

struct Material
{
    int baseColorTexIndex;
    int metallicTexIndex;
    int roughnessTexIndex;
    int normalTexIndex;
    int aoTexIndex;
    int emissionTexIndex;
    float metallicFactor;
    float roughnessFactor;
    float occlusionStrength;
    float emissionFactor;
    float alphaCutoff;
    uint alphaMode;
    vec4 baseColorFactor;
    vec4 emissionColor;
    vec2 tiling;
    vec2 offset;
};
