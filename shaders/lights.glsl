
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

const uint NoShadow = 0;
const uint HardShadow = 1;
const uint SoftShadow = 2;

struct PointShadowData
{
    mat4 viewProj[6];
    uint shadowType;
    uint resolution;
    float strength;
    float biasSlope;
    float biasConstant;
    float pcfRadius;
    uint padding[2];
};

struct SpotShadowData
{
    mat4 viewProj;
    uint shadowType;
    uint resolution;
    float strength;
    float biasSlope;
    float biasConstant;
    int pcfRange;
    uint padding[2];
};

float calcAttenuation(float distance, float lightRange)
{
    float t = clamp(distance / lightRange, 0.0, 1.0);
    return 1.0 - smoothstep(0.0, 1.0, t);
}

const vec3 sampleOffsets3D[20] = vec3[20]
(
    vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1),
    vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
    vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
    vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
    vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
);
