#version 460 core

#define PI 3.141592653589793

layout (location = 0) in vec3 vTexCoords;

layout (location = 0) out vec4 outFragColor;

layout (set = 0, binding = 1) uniform samplerCube envMap;

void main()
{
    vec3 normal = normalize(vTexCoords);
    vec3 up = vec3(0.0, 1.0, 0.0);
    vec3 right = normalize(cross(up, normal));
    up = normalize(cross(normal, right));

    vec3 irradiance = vec3(0.0);
    float angleDt = 0.025;
    float sampleCount = 0.0;

    for (float phi = 0.0; phi < 2.0 * PI; phi += angleDt)
    {
        for (float theta = 0.0; theta < PI / 2.0; theta += angleDt)
        {
            vec3 tangent = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
            vec3 sampleVec = tangent.x * right + tangent.y * up + tangent.z * normal;
            irradiance += texture(envMap, sampleVec).rgb * cos(theta) * sin(theta);
            ++sampleCount;
        }
    }

    irradiance = PI * irradiance / sampleCount;

    outFragColor = vec4(irradiance, 1.0);
}
