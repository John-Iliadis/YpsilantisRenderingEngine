#version 460 core

#include "material.glsl"
#include "lights.glsl"
#include "rendering_equations.glsl"
#include "cluster.glsl"

#define MAX_SHADOW_MAPS_PER_TYPE 50

layout (early_fragment_tests) in;

layout (location = 0) in vec3 vFragWorldPos;
layout (location = 1) in vec2 vTexCoords;
layout (location = 2) in mat3 vTBN;

layout (location = 0) out vec4 outFragColor;

layout (push_constant) uniform PushConstants
{
    uint dirLightCount;
    uint pointLightCount;
    uint spotLightCount;
    uint debugNormals;
    uint screenWidth;
    uint screenHeight;
    uint clusterGridX;
    uint clusterGridY;
    uint clusterGridZ;
    uint enableIblLighting;
};

layout (set = 0, binding = 0) uniform CameraUBO
{
    mat4 view;
    mat4 projection;
    mat4 viewProj;
    vec4 cameraPos;
    vec4 cameraDir;
    float nearPlane;
    float farPlane;
};

layout (set = 1, binding = 0) uniform sampler2D ssaoTexture;
layout (set = 1, binding = 1) buffer readonly ClustersSSBO { Cluster clusters[]; };
layout (set = 1, binding = 2) uniform sampler2D viewPosTexture;
layout (set = 1, binding = 3) uniform samplerCube irradianceMap;
layout (set = 1, binding = 4) uniform samplerCube prefilterMap;
layout (set = 1, binding = 5) uniform sampler2D brdfLut;

layout (set = 2, binding = 0) buffer readonly DirLightsSSBO { DirectionalLight dirLights[]; };
layout (set = 2, binding = 1) buffer readonly PointLightsSSBO { PointLight pointLights[]; };
layout (set = 2, binding = 2) buffer readonly SpotLightSSBO { SpotLight spotLights[]; };
layout (set = 2, binding = 3) buffer readonly DirShadowDataSSBO { DirShadowData dirShadowData[]; };
layout (set = 2, binding = 4) buffer readonly PointShadowDataSSBO { PointShadowData pointShadowData[]; };
layout (set = 2, binding = 5) buffer readonly SpotShadowDataSSBO { SpotShadowData spotShadowData[]; };
layout (set = 2, binding = 6) uniform sampler dirLightSampler;
layout (set = 2, binding = 7) uniform sampler pointShadowSampler;
layout (set = 2, binding = 8) uniform sampler spotShadowMapSampler;
layout (set = 2, binding = 9) uniform texture2DArray dirShadowMaps[MAX_SHADOW_MAPS_PER_TYPE];
layout (set = 2, binding = 10) uniform textureCube pointShadowMaps[MAX_SHADOW_MAPS_PER_TYPE];
layout (set = 2, binding = 11) uniform texture2D spotShadowMaps[MAX_SHADOW_MAPS_PER_TYPE];

layout (set = 3, binding = 0) uniform MaterialsUBO { Material material; };
layout (set = 3, binding = 1) uniform sampler2D baseColorTex;
layout (set = 3, binding = 2) uniform sampler2D metallicTex;
layout (set = 3, binding = 3) uniform sampler2D roughnessTex;
layout (set = 3, binding = 4) uniform sampler2D normalTex;
layout (set = 3, binding = 5) uniform sampler2D aoTex;
layout (set = 3, binding = 6) uniform sampler2D emissionTex;

const float maxReflectionLod = 5.0;

uint getClusterIndex(vec3 viewPos)
{
    vec2 clusterSizeXY = vec2(screenWidth, screenHeight) / vec2(clusterGridX, clusterGridY);
    uvec3 cluster = uvec3(gl_FragCoord.xy / clusterSizeXY, 0);
    cluster.z = uint(clusterGridZ * log(-viewPos.z / nearPlane) / log(farPlane / nearPlane));

    return cluster.x +
           cluster.y * clusterGridX +
           cluster.z * clusterGridX * clusterGridY;
}

float dirShadowCalculation(uint index, vec3 viewPos, vec3 normal, vec3 lightDir)
{
    float depth = abs(viewPos.z);

    uint layer = dirShadowData[index].cascadeCount - 1;
    for (uint i = 0; i < dirShadowData[index].cascadeCount; ++i)
    {
        if (depth < dirShadowData[index].cascadeDist[i])
        {
            layer = i;
            break;
        }
    }

    vec4 fragPosLightSpace = dirShadowData[index].viewProj[layer] * vec4(vFragWorldPos, 1.0); // light space
    fragPosLightSpace /= fragPosLightSpace.w; // NDC

    float currentDepth = fragPosLightSpace.z;
    vec2 sampleCoords = fragPosLightSpace.xy * 0.5 + 0.5; // to range [0, 1]

    if (currentDepth > 1.0)
        return 0.0;

    float bias = max(dirShadowData[index].biasSlope * (1.0 - dot(normal, lightDir)), dirShadowData[index].biasConstant);
    bias *= 1.0 / (dirShadowData[index].cascadeDist[layer] * 0.5);

    float shadow = 0.0;
    if (dirShadowData[index].shadowType == SoftShadow)
    {
        int pcfRange = dirShadowData[index].pcfRange;
        float texelSize = 1.0 / dirShadowData[index].resolution;

        for (int x = -pcfRange; x <= pcfRange; ++x)
        {
            for (int y = -pcfRange; y <= pcfRange; ++y)
            {
                vec2 uv = sampleCoords + vec2(x, y) * texelSize;
                float pcfDepth = texture(sampler2DArray(dirShadowMaps[index], dirLightSampler), vec3(uv, layer)).r;
                shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
            }
        }

        shadow /= float(pow(pcfRange * 2 + 1, 2.0));
    }
    else
    {
        float sampleDepth = texture(sampler2DArray(dirShadowMaps[index], dirLightSampler), vec3(sampleCoords, layer)).r;
        shadow = currentDepth - bias > sampleDepth? 1.0 : 0.0;
    }

    return shadow;
}

float pointShadowCalculation(uint index, vec3 normal, vec3 lightDir)
{
    vec3 lightToFrag = vFragWorldPos - pointLights[index].position.xyz;
    float currentDepth = length(lightToFrag);
    float bias = max(pointShadowData[index].biasSlope * (1.0 - dot(normal, lightDir)), pointShadowData[index].biasConstant);
    float shadow;

    if (pointShadowData[index].shadowType == SoftShadow)
    {
        int range = 2;
        float texelSize = 1.0 / pointShadowData[index].resolution;

        for (int x = -range; x <= range; ++x)
        for (int y = -range; y <= range; ++y)
        for (int z = -range; z <= range; ++z)
        {
            vec3 sampleDir = lightToFrag + vec3(x, y, z) * (texelSize * pointShadowData[index].pcfRadius);
            float sampleDepth = texture(samplerCube(pointShadowMaps[index], pointShadowSampler), sampleDir).r;
            sampleDepth *= pointLights[index].range;
            shadow += currentDepth - bias > sampleDepth? 1.0 : 0.0;
        }

        shadow /= pow(range * 2 + 1, 3.0);
    }
    else
    {
        float sampleDepth = texture(samplerCube(pointShadowMaps[index], pointShadowSampler), lightToFrag).r;
        sampleDepth *= pointLights[index].range;
        shadow = currentDepth - bias > sampleDepth? 1.0 : 0.0;
    }

    return shadow;
}

float spotShadowCalculation(uint index, vec3 normal, vec3 lightDir)
{
    // calculate the frag position in light space
    vec4 fragPosLightSpace = spotShadowData[index].viewProj * vec4(vFragWorldPos, 1.0); // light space
    fragPosLightSpace /= fragPosLightSpace.w; // NDC

    vec2 sampleCoords = fragPosLightSpace.xy * 0.5 + 0.5; // to range [0, 1]
    float currentDepth = fragPosLightSpace.z;
    float bias = max(spotShadowData[index].biasSlope * (1.0 - dot(normal, lightDir)), spotShadowData[index].biasConstant);
    float shadow = 0.0;

    if (spotShadowData[index].shadowType == SoftShadow)
    {
        int pcfRange = spotShadowData[index].pcfRange;
        float texelSize = 1.0 / spotShadowData[index].resolution;

        for (int x = -pcfRange; x <= pcfRange; ++x)
        {
            for (int y = -pcfRange; y <= pcfRange; ++y)
            {
                vec2 uv = sampleCoords + vec2(x, y) * texelSize;
                float pcfDepth = texture(sampler2D(spotShadowMaps[index], spotShadowMapSampler), uv).r;
                shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
            }
        }

        shadow /= float(pow(pcfRange * 2 + 1, 2.0));
    }
    else
    {
        float sampleDepth = texture(sampler2D(spotShadowMaps[index], spotShadowMapSampler), sampleCoords).r;
        shadow = currentDepth - bias > sampleDepth? 1.0 : 0.0;
    }

    return shadow;
}

void main()
{
    vec2 texCoords = vTexCoords * material.tiling + material.offset;
    vec2 screenSpaceTexCoords = gl_FragCoord.xy / vec2(float(screenWidth), float(screenHeight));

    vec4 baseColor = texture(baseColorTex, texCoords) * material.baseColorFactor;

    if (material.alphaMode == AlphaModeMask && baseColor.a < material.alphaCutoff)
        discard;

    float metallic = texture(metallicTex, texCoords).b * material.metallicFactor;
    float roughness = texture(roughnessTex, texCoords).g * material.roughnessFactor;
    vec3 normalSample = normalize(texture(normalTex, texCoords).xyz * 2.0 - 1.0);
    float ao = 1.0 + material.occlusionStrength * (texture(aoTex, texCoords).r - 1.0);
    vec3 emission = texture(emissionTex, texCoords).rgb * material.emissionColor.rgb * material.emissionFactor;
    float occlusionFactor = texture(ssaoTexture, screenSpaceTexCoords).r;
    vec3 viewPos = texture(viewPosTexture, screenSpaceTexCoords).xyz;

    float hasNormalMap = float(material.normalTexIndex != -1);
    vec3 normal = vTBN[2] * (1.0 - hasNormalMap) + vTBN * normalSample * hasNormalMap;
    normal = normalize(normal);
    vec3 viewVec = normalize(cameraPos.xyz - vFragWorldPos);
    vec3 R = reflect(-viewVec, normal);

    vec3 L_0 = vec3(0.0);
    vec3 F_0 = mix(vec3(0.04), baseColor.xyz, metallic);

    for (uint i = 0; i < dirLightCount; ++i)
    {
        vec3 lightVec = -normalize(dirLights[i].direction.xyz);
        vec3 halfwayVec = normalize(viewVec + lightVec);
        vec3 lightRadiance = dirLights[i].color.rgb * dirLights[i].intensity;
        vec3 lightContribution = specularContribution(lightVec, viewVec, normal, F_0, lightRadiance, baseColor.xyz, metallic, roughness);

        if (dirShadowData[i].shadowType != NoShadow)
        {
            float shadow = dirShadowCalculation(i, viewPos, normal, lightVec);
            lightContribution *= 1 - (shadow * dirShadowData[i].strength);
        }

        L_0 += lightContribution;
    }

    uint clusterIndex = getClusterIndex(viewPos);
    Cluster cluster = clusters[clusterIndex];
    for (uint i = 0; i < cluster.lightCount; ++i)
    {
        PointLight pointLight = pointLights[cluster.lightIndices[i]];

        vec3 lightToPosVec = pointLight.position.xyz - vFragWorldPos;
        float dist = length(lightToPosVec);
        float attenuation = calcAttenuation(dist, pointLight.range);
        vec3 lightVec = normalize(lightToPosVec);
        vec3 lightRadiance = pointLight.color.rgb * (pointLight.intensity * attenuation);
        vec3 halfwayVec = normalize(viewVec + lightVec);
        vec3 lightContribution = specularContribution(lightVec, viewVec, normal, F_0, lightRadiance, baseColor.xyz, metallic, roughness);

        if (pointShadowData[i].shadowType != NoShadow)
        {
            float shadow = pointShadowCalculation(i, normal, lightVec);
            lightContribution *= 1 - (shadow * pointShadowData[i].strength);
        }

        L_0 += lightContribution;
    }

    for (uint i = 0; i < spotLightCount; ++i)
    {
        float innerCutoff = cos((spotLights[i].innerCutoff / 2.0) * PI / 180.0);
        float outerCutoff = cos((spotLights[i].outerCutoff / 2.0) * PI / 180.0);

        vec3 posToLightVec = spotLights[i].position.xyz - vFragWorldPos;
        vec3 lightVec = normalize(posToLightVec);
        float dist = length(posToLightVec);
        float cosTheta = dot(normalize(-spotLights[i].direction.xyz), lightVec);
        float epsilon = innerCutoff - outerCutoff;
        float intensity = clamp((cosTheta - outerCutoff) / epsilon, 0.0, 1.0);
        float attenuation = calcAttenuation(dist, spotLights[i].range);
        vec3 lightRadiance = spotLights[i].color.rgb * (spotLights[i].intensity * intensity * attenuation);
        vec3 halfwayVec = normalize(lightVec + viewVec);
        vec3 lightContribution = specularContribution(lightVec, viewVec, normal, F_0, lightRadiance, baseColor.xyz, metallic, roughness);

        if (spotShadowData[i].shadowType != NoShadow)
        {
            float shadow = spotShadowCalculation(i, normal, lightVec);
            lightContribution *= 1 - (shadow * spotShadowData[i].strength);
        }

        L_0 += lightContribution;
    }

    vec3 ambient;
    if (enableIblLighting == 1)
    {
        vec2 brdf = texture(brdfLut, vec2(max(dot(normal, viewVec), 0.0), roughness)).rg;
        vec3 irradiance = texture(irradianceMap, normal).rgb;

        float lod = roughness * maxReflectionLod;
        float lodf = floor(lod);
        float lodc = ceil(lod);
        vec3 a = textureLod(prefilterMap, R, lodf).rgb;
        vec3 b = textureLod(prefilterMap, R, lodc).rgb;
        vec3 reflection = mix(a, b, lod - lodf);

        vec3 diffuse = irradiance * baseColor.xyz;

        vec3 F = F_SchlickR(max(dot(normal, viewVec), 0.0), F_0, roughness);
        vec3 specular = reflection * (F * brdf.x + brdf.y);

        vec3 kD = 1.0 - F;
        kD *= 1.0 - metallic;

        ambient = (kD * diffuse + specular) * ao;
    }
    else
    {
        ambient = vec3(0.1) * baseColor.xyz * ao;
    }

    outFragColor = vec4(emission + L_0 + ambient, 1.0) * occlusionFactor;
    outFragColor.a = baseColor.a;

    if (debugNormals == 1)
    {
        outFragColor = vec4(normal, 1.0);
    }
}
