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
    float angleDt = 0.015;
    float sampleCount = 0.0;

    const float twoPi = 2.0 * PI;
    const float halfPi = PI / 2.0;

    for (float phi = 0.0; phi < twoPi; phi += angleDt)
    {
        for (float theta = 0.0; theta < halfPi; theta += angleDt)
        {
            vec3 tempVec = cos(phi) * right + sin(phi) * up;
            vec3 sampleVector = cos(theta) * normal + sin(theta) * tempVec;
            irradiance += texture(envMap, sampleVector).rgb * cos(theta) * sin(theta);
            ++sampleCount;
        }
    }

    outFragColor = vec4(PI * irradiance / sampleCount, 1.0);
}
