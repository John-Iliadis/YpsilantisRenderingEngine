
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
     float k_ibl = (roughness * roughness) / 2.0;

    return NdotV / (NdotV * (1 - k_ibl) + k_ibl);
}

float geometrySmith(vec3 normal, vec3 viewVec, vec3 lightVec, float roughness)
{
    float NdotV = max(dot(normal, viewVec), 0.0);
    float NdotL = max(dot(normal, lightVec), 0.0);

    return geometrySchlickGGX(NdotV, roughness) *
           geometrySchlickGGX(NdotL, roughness);
}

vec3 fresnelSchlick(vec3 viewVec, vec3 halfwayVec, vec3 F_0, float roughness)
{
    float VdotH = max(dot(viewVec, halfwayVec), 0.0);

    return F_0 + (max(vec3(1.0 - roughness), F_0) - F_0) * pow(clamp(1.0 - VdotH, 0.0, 1.0), 5.0);
}

vec3 renderingEquation(vec3 normal, vec3 lightVec, vec3 viewVec, vec3 halfwayVec,
          vec3 lightRadiance, vec3 baseColor, vec3 F_0, float metallic, float roughness)
{
    float distribution = distributionGGX(normal, halfwayVec, roughness);
    float geometry = geometrySmith(normal, viewVec, lightVec, roughness);
    vec3 frensel = fresnelSchlick(viewVec, halfwayVec, F_0, roughness);

    vec3 numerator = (distribution * geometry) * frensel;
    float denominator = 4 * max(dot(viewVec, normal), 0.0) * max(dot(lightVec, normal), 0.0) + 1e-5;
    vec3 specular = numerator / denominator;

    vec3 kS = frensel;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    float NdotL = max(dot(normal, lightVec), 0.0);

    return (kD * baseColor / PI + specular) * lightRadiance * NdotL;
}

float radicalInverseVDC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}

vec2 hammersley(uint i, uint n)
{
    return vec2(float(i) / float(n), radicalInverseVDC(i));
}

vec3 importanceSampleGGX(vec2 xi, vec3 normal, float roughness)
{
    float a = roughness * roughness;

    float phi = 2.0 * PI * xi.x;
    float cosTheta = sqrt((1.0 - xi.y) / (1.0 + (a * a - 1.0) * xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    vec3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;

    vec3 up = abs(normal.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(up, normal));
    vec3 bitangent = cross(normal, tangent);

    vec3 sampleVec = tangent * H.x + bitangent * H.y + normal * H.z;

    return normalize(sampleVec);
}

vec2 integrateBRDF(float NdotV, float roughness, uint sampleCount)
{
    vec3 V;
    V.x = sqrt(1.0 - NdotV * NdotV);
    V.y = 0.0;
    V.z = NdotV;

    float A = 0.0;
    float B = 0.0;

    vec3 N = vec3(0.0, 0.0, 1.0);

    for(uint i = 0u; i < sampleCount; ++i)
    {
        vec2 Xi = hammersley(i, sampleCount);
        vec3 H = importanceSampleGGX(Xi, N, roughness);
        vec3 L = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(L.z, 0.0);
        float NdotH = max(H.z, 0.0);
        float VdotH = max(dot(V, H), 0.0);

        if(NdotL > 0.0)
        {
            float G = geometrySmith(N, V, L, roughness);
            float G_Vis = (G * VdotH) / (NdotH * NdotV);
            float Fc = pow(1.0 - VdotH, 5.0);

            A += (1.0 - Fc) * G_Vis;
            B += Fc * G_Vis;
        }
    }

    A /= float(sampleCount);
    B /= float(sampleCount);

    return vec2(A, B);
}
