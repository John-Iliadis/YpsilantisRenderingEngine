#version 460 core

#define PI 3.141592653589793

layout (location = 0) in vec3 vTexCoords;

layout (location = 0) out vec4 outFragColor;

layout (set = 0, binding = 1) uniform samplerCube envMap;

void main()
{
    vec3 N = normalize(vTexCoords);
    vec3 up = vec3(0.0, 1.0, 0.0);
    vec3 R = normalize(cross(up, N));
    vec3 U = normalize(cross(N, R));

    const float twoPi = 2.0 * PI;
    const float halfPi = PI / 2.0;
    const float angleStep = 0.02;

    vec3 irradiance = vec3(0.0);
    float sampleCount = 0.0;

    for (float phi = 0.0; phi < twoPi; phi += angleStep)
    {
        float cosPhi = cos(phi);
        float sinPhi = sin(phi);
        vec3 H = normalize(cosPhi * R + sinPhi * U);

        for (float theta = 0.0; theta < halfPi; theta += angleStep)
        {
            float cosTheta = cos(theta);
            float sinTheta = sin(theta);

            vec3 sampleVector = cosTheta * N + sinTheta * H;
            irradiance += texture(envMap, sampleVector).rgb * cosTheta * sinTheta;
            ++sampleCount;
        }
    }

    outFragColor = vec4(PI * irradiance / sampleCount, 1.0);
}
