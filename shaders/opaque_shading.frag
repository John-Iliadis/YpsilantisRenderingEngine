#version 460 core

#include "material.glsl"
#include "lights.glsl"
#include "rendering_equations.glsl"
#include "cluster.glsl"

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

void main()
{
    vec2 texCoords = vTexCoords * material.tiling + material.offset;
    vec2 screenSpaceTexCoords = gl_FragCoord.xy / vec2(float(screenWidth), float(screenHeight));

    vec3 baseColor = texture(baseColorTex, texCoords).rgb * material.baseColorFactor.rgb;
    float metallic = texture(metallicTex, texCoords).b * material.metallicFactor;
    float roughness = texture(roughnessTex, texCoords).g * material.roughnessFactor;
    vec3 normalSample = normalize(texture(normalTex, texCoords).xyz * 2.0 - 1.0);
    float ao = texture(aoTex, texCoords).r;
    vec3 emission = texture(emissionTex, texCoords).rgb * material.emissionColor.rgb * material.emissionFactor;
    float occlusionFactor = texture(ssaoTexture, screenSpaceTexCoords).r;
    vec3 viewPos = texture(viewPosTexture, screenSpaceTexCoords).xyz;

    vec3 normal = normalize(vTBN * normalSample);
    vec3 viewVec = normalize(cameraPos.xyz - vFragWorldPos);
    vec3 R = reflect(-viewVec, normal);

    vec3 L_0 = vec3(0.0);
    vec3 F_0 = mix(vec3(0.04), baseColor, metallic);

    for (uint i = 0; i < dirLightCount; ++i)
    {
        vec3 lightVec = -normalize(dirLights[i].direction.xyz);
        vec3 halfwayVec = normalize(viewVec + lightVec);
        vec3 lightRadiance = dirLights[i].color.rgb * dirLights[i].intensity;

        L_0 += renderingEquation(normal, lightVec, viewVec, halfwayVec, lightRadiance, baseColor, F_0, metallic, roughness);
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

        L_0 += renderingEquation(normal, lightVec, viewVec, halfwayVec, lightRadiance, baseColor, F_0, metallic, roughness);
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

        L_0 += renderingEquation(normal, lightVec, viewVec, halfwayVec, lightRadiance, baseColor, F_0, metallic, roughness);
    }

    vec3 ambient;
    if (enableIblLighting == 1)
    {
        vec3 F = fresnelSchlickRoughness(normal, viewVec, F_0, roughness);
        vec3 kS = F;
        vec3 kD = 1.0 - kS;
        kD *= 1.0 - metallic;

        vec3 irradiance = texture(irradianceMap, normal).rgb;
        vec3 diffuse = irradiance * baseColor;

        vec3 prefilterColor = textureLod(prefilterMap, R, roughness * maxReflectionLod).rgb;
        vec2 brdf = texture(brdfLut, vec2(max(dot(normal, viewVec), 0.0), roughness)).rg;
        vec3 specular = prefilterColor * (F * brdf.x + brdf.y);

        ambient = (kD * diffuse + specular) * ao;
    }
    else
    {
        ambient = vec3(0.1) * baseColor * ao;
    }

    outFragColor = vec4(emission + L_0 + ambient, 1.0) * occlusionFactor * (1.0 - float(debugNormals)) +
                   vec4(normal, 1.0) * float(debugNormals);
}
