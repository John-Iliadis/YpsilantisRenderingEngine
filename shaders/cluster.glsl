
#define MAX_LIGHTS_PER_CLUSTER 32

struct Cluster
{
    vec4 minPoint;
    vec4 maxPoint;
    uint lightCount;
    uint lightIndices[MAX_LIGHTS_PER_CLUSTER];
};
