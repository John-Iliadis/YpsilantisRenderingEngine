
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
    vec4 baseColorFactor;
    vec4 emissionFactor;
    vec2 tiling;
    vec2 offset;
};
