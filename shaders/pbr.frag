#version 460 core

#include "material.glsl"
#include "lights.glsl"
#include "brdf.glsl"

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

// todo: put brdf calculation in a function
// todo: issues with metalness
// todo: optimize vector miltiplication order
// PBR requires all inputs to be linear
void main()
{
    vec2 texCoords = vTexCoords * material.tiling + material.offset;

    vec3 baseColor = texture(baseColorTex, texCoords).rgb * material.baseColorFactor.rgb;
    float metallic = texture(metallicTex, texCoords).b * material.metallicFactor;
    float roughness = texture(roughnessTex, texCoords).g * material.roughnessFactor;
    vec3 normalSample = normalize(texture(normalTex, texCoords).xyz * 2.0 - 1.0);
    float ao = texture(aoTex, texCoords).r;
    vec3 emission = texture(emissionTex, texCoords).rgb * material.emissionFactor.rgb;

    vec3 normal = normalize(vTBN * normalSample);
    vec3 viewVec = normalize(cameraPos.xyz - vFragWorldPos);

    vec3 L_0 = vec3(0.0);
    vec3 F_0 = mix(vec3(0.04), baseColor, metallic);

    for (uint i = 0; i < dirLightCount; ++i)
    {
        vec3 lightVec = -normalize(dirLights[i].direction.xyz);
        vec3 halfwayVec = normalize(viewVec + lightVec);
        vec3 lightRadiance = dirLights[i].color.rgb * dirLights[i].intensity;

        float distribution = distributionGGX(normal, halfwayVec, roughness);
        float geometry = geometrySmith(normal, viewVec, lightVec, roughness);
        vec3 frensel = fresnelSchlick(viewVec, halfwayVec, F_0);

        vec3 BRDF_Numerator = distribution * geometry * frensel;
        float BRDF_Denominator = 4 * max(dot(viewVec, normal), 0.0) * max(dot(lightVec, normal), 0.0) + 1e-5;
        vec3 BRDF = BRDF_Numerator / BRDF_Denominator;

        vec3 diffuseTerm = baseColor * (vec3(1.0) - frensel) * (1.0 - metallic) / PI;

        L_0 += (diffuseTerm + BRDF) * lightRadiance * max(dot(normal, lightVec), 0.0);
    }

    for (uint i = 0; i < pointLightCount; ++i)
    {
        vec3 lightToPosVec = pointLights[i].position.xyz - vFragWorldPos;
        float dist = length(lightToPosVec);
        float attenuation = calcAttenuation(dist, pointLights[i].range);
        vec3 lightVec = normalize(lightToPosVec);
        vec3 lightRadiance = pointLights[i].color.rgb * pointLights[i].intensity * attenuation;
        vec3 halfwayVec = normalize(viewVec + lightVec);

        float distribution = distributionGGX(normal, halfwayVec, roughness);
        float geometry = geometrySmith(normal, viewVec, lightVec, roughness);
        vec3 frensel = fresnelSchlick(viewVec, halfwayVec, F_0);

        vec3 BRDF_Numerator = distribution * geometry * frensel;
        float BRDF_Denominator = 4 * max(dot(viewVec, normal), 0.0) * max(dot(lightVec, normal), 0.0) + 1e-5;
        vec3 BRDF = BRDF_Numerator / BRDF_Denominator;

        vec3 diffuseTerm = baseColor * (vec3(1.0) - frensel) * (1.0 - metallic) / PI;

        L_0 += (diffuseTerm + BRDF) * lightRadiance * max(dot(normal, lightVec), 0.0);
    }

    // currently cutoffs include the whole angle
    for (uint i = 0; i < spotLightCount; ++i)
    {
        float innerCutoff = cos((spotLights[i].innerCutoff / 2.0) * PI / 180.0);
        float outerCutoff = cos((spotLights[i].outerCutoff / 2.0) * PI / 180.0);

        vec3 posToLightVec = spotLights[i].position.xyz - vFragWorldPos;
        vec3 lightVec = normalize(posToLightVec);
        float dist = length(posToLightVec);
        float cosTheta = dot(normalize(-spotLights[i].direction.xyz), lightVec); // -spotLights[i].direction.xyz
        float epsilon = innerCutoff - outerCutoff;
        float intensity = clamp((cosTheta - outerCutoff) / epsilon, 0.0, 1.0);
        float attenuation = calcAttenuation(dist, spotLights[i].range);
        vec3 lightRadiance = spotLights[i].color.rgb * spotLights[i].intensity * intensity * attenuation;
        vec3 halfwayVec = normalize(lightVec + viewVec);

        float distribution = distributionGGX(normal, halfwayVec, roughness);
        float geometry = geometrySmith(normal, viewVec, lightVec, roughness);
        vec3 frensel = fresnelSchlick(viewVec, halfwayVec, F_0);

        vec3 BRDF_Numerator = distribution * geometry * frensel;
        float BRDF_Denominator = 4 * max(dot(viewVec, normal), 0.0) * max(dot(lightVec, normal), 0.0) + 1e-5;
        vec3 BRDF = BRDF_Numerator / BRDF_Denominator;

        vec3 diffuseTerm = baseColor * (vec3(1.0) - frensel) * (1.0 - metallic) / PI;

        L_0 += (diffuseTerm + BRDF) * lightRadiance * max(dot(normal, lightVec), 0.0);
    }

    vec3 ambient = vec3(0.1) * baseColor * ao;

    outFragColor = vec4(L_0 + ambient, 1.0) * (1.0 - float(debugNormals)) +
                   vec4(normal * 0.5 + 0.5, 1.0) * float(debugNormals);
}
