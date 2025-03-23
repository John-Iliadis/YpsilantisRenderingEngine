#version 460 core

layout (location = 0) in vec2 vTexCoords;

layout (location = 0) out float outOcclusionFactor;

layout (push_constant) uniform PushConstants
{
    uint kernelSize;
    float radius;
    float intensity;
    float bias;
    vec2 screenSize;
    float noiseTextureSize;
};

layout (set = 0, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 projection;
    mat4 viewProj;
    vec4 cameraPos;
    vec4 cameraDir;
    float nearPlane;
    float farPlane;
};

layout (set = 1, binding = 0) uniform sampler2D posTexture;
layout (set = 1, binding = 1) uniform sampler2D normalTexture;
layout (set = 1, binding = 2) uniform sampler2D noiseTexture;
layout (set = 1, binding = 3) readonly buffer KernelSSBO { vec4 ssaoKernel[]; };

void main()
{
    vec4 position = texture(posTexture, vTexCoords);
    vec3 normal = texture(normalTexture, vTexCoords).xyz;
    vec3 randomVec = texture(noiseTexture, vTexCoords * screenSize / noiseTextureSize).xyz;

    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = normalize(cross(normal, tangent));
    mat3 TBN = mat3(tangent, bitangent, normal);

    float occlusionFactor = 0.0;

    for (uint i = 0; i < kernelSize; ++i)
    {
        vec3 offsetPos = position.xyz + TBN * ssaoKernel[i].xyz * radius;

        vec4 offsetPosSS = projection * vec4(offsetPos, 1.0);
        offsetPosSS.xyz /= offsetPosSS.w;
        offsetPosSS.xyz = offsetPosSS.xyz * 0.5 + 0.5;

        float currentDepth = texture(posTexture, offsetPosSS.xy).z;

        occlusionFactor += smoothstep(0.0, 1.0, radius / abs(position.z - currentDepth))
                           * float(currentDepth > offsetPos.z + bias);
    }

    outOcclusionFactor = clamp(0.0, 1.0, 1.0 - ((occlusionFactor / kernelSize) * intensity));
}
