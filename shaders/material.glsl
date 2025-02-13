struct Material
{
    int baseColorTexIndex;
    int metallicRoughnessTexIndex;
    int normalTexIndex;
    int aoTexIndex;
    int emissionTexIndex;
    float metallicFactor;
    float roughnessFactor;
    float occlusionFactor;
    vec4 baseColorFactor;
    vec4 emissionFactor;
    vec2 tiling;
    vec2 offset;
};
