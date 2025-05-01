#version 460 core

#define PI 3.1415926535897932384626433832795

layout (location = 0) in vec3 vTexCoords;

layout (location = 0) out vec4 outFragColor;

layout (set = 0, binding = 1) uniform samplerCube envMap;

const float TWO_PI = PI * 2.0;
const float HALF_PI = PI * 0.5;
const float SAMPLE_DT = 0.025;

void main()
{
    vec3 N = normalize(vTexCoords);
    vec3 up = vec3(0.0, 1.0, 0.0);
    vec3 right = normalize(cross(up, N));
    up = cross(N, right);

    vec3 color = vec3(0.0);
    uint sampleCount = 0u;
    for (float phi = 0.0; phi < TWO_PI; phi += SAMPLE_DT)
    {
        for (float theta = 0.0; theta < HALF_PI; theta += SAMPLE_DT)
        {
            vec3 tempVec = cos(phi) * right + sin(phi) * up;
            vec3 sampleVector = cos(theta) * N + sin(theta) * tempVec;
            color += texture(envMap, sampleVector).rgb * cos(theta) * sin(theta);
            sampleCount++;
        }
    }

    outFragColor = vec4(PI * color / float(sampleCount), 1.0);
}
