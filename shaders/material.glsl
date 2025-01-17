#define METALLIC_WORKFLOW 1
#define SPECULAR_WORKFLOW 2

struct Material
{
    uint workflow;
    uint albedoMapIndex;
    uint roughnessMapIndex;
    uint metallicMapIndex;
    uint normalMapIndex;
    uint heightMapIndex;
    uint aoMapIndex;
    uint emissionMapIndex;
    uint specularMapIndex;
    vec4 albedoColor;
    vec4 emissionColor;
    vec2 tiling;
    vec2 offset;
};
