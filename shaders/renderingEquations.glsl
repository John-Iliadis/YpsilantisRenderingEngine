
#define PI 3.14159265359

float distributionGGX(vec3 normal, vec3 halfwayVec, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;

    float NdotH2 = pow(max(dot(normal, halfwayVec), 0.0), 2.0);

    float denom = PI * pow(NdotH2 * (a2 - 1.0) + 1.0, 2.0);

    return a2 / denom;
}

float geometrySchlickGGX(float NdotV, float roughness)
{
    float k_direct = pow(roughness + 1.0, 2.0) / 8.0;

    // todo: check
    // float k_ibl = (roughness * roughness) / 2.0;

    return NdotV / (NdotV * (1 - k_direct) + k_direct);
}

float geometrySmith(vec3 normal, vec3 viewVec, vec3 lightVec, float roughness)
{
    float NdotV = max(dot(normal, viewVec), 0.0);
    float NdotL = max(dot(normal, lightVec), 0.0);

    return geometrySchlickGGX(NdotV, roughness) *
           geometrySchlickGGX(NdotL, roughness);
}

vec3 fresnelSchlick(vec3 viewVec, vec3 halfwayVec, vec3 F_0)
{
    float VdotH = max(dot(viewVec, halfwayVec), 0.0);

    return F_0 + (1 - F_0) * pow(1 - VdotH, 5.0);
}

vec3 renderingEquation(vec3 normal, vec3 lightVec, vec3 viewVec, vec3 halfwayVec,
          vec3 lightRadiance, vec3 baseColor, vec3 F_0, float metallic, float roughness)
{
    float distribution = distributionGGX(normal, halfwayVec, roughness);
    float geometry = geometrySmith(normal, viewVec, lightVec, roughness);
    vec3 frensel = fresnelSchlick(viewVec, halfwayVec, F_0);

    vec3 numerator = (distribution * geometry) * frensel;
    float denominator = 4 * max(dot(viewVec, normal), 0.0) * max(dot(lightVec, normal), 0.0) + 1e-5;
    vec3 specular = numerator / denominator;

    vec3 diffuseTerm = baseColor * (vec3(1.0) - frensel) * (1.0 - metallic) / PI;

    return (diffuseTerm + specular) * lightRadiance * max(dot(normal, lightVec), 0.0);
}
