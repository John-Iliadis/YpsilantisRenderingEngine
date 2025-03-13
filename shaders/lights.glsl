
struct DirectionalLight
{
    vec4 color;
    vec4 direction;
    float intensity;
};

struct PointLight
{
    vec4 color;
    vec4 position;
    float intensity;
    float range;
};

struct SpotLight
{
    vec4 color;
    vec4 position;
    vec4 direction;
    float intensity;
    float range;
    float innerCutoff;
    float outerCutoff;
};

float calcAttenuation(float distance, float lightRange)
{
    return pow(1 - distance / lightRange, 2.0) * float(distance < lightRange);
}
