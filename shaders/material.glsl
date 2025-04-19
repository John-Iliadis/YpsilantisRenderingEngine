
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
    float emissionFactor;
    float alphaMask;
    float alphaCutoff;
    vec4 baseColorFactor;
    vec4 emissionColor;
    vec2 tiling;
    vec2 offset;
};
