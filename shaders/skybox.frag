#version 460 core

layout (early_fragment_tests) in;

layout (location = 0) in vec3 vTexCoords;

layout (location = 0) out vec4 outFragColor;

layout (push_constant) uniform PushConstants
{
    int flipX;
    int flipY;
    int flipZ;
};

layout (set = 1, binding = 0) uniform samplerCube cubemap;

void main()
{
    vec3 texCoords = vec3(
        vTexCoords.x * mix(1.0, -1.0, float(flipX)),
        vTexCoords.y * mix(1.0, -1.0, float(flipY)),
        vTexCoords.z * mix(1.0, -1.0, float(flipZ))
    );

    outFragColor = texture(cubemap, texCoords);
}
