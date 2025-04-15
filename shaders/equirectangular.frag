#version 460 core

#define PI 3.141592653589793

layout (location = 0) in vec3 vTexCoords;

layout (location = 0) out vec4 outFragColor;

layout (set = 0, binding = 1) uniform sampler2D equirectangularMap;

void main()
{
    vec3 dir = normalize(vTexCoords);

    float theta = atan(dir.x, dir.z); // U
    float phi = asin(dir.y); // V

    theta = (theta + PI) / (2.0 * PI);
    phi = (phi + PI / 2.0) / PI;

    outFragColor = texture(equirectangularMap, vec2(theta, phi));
}
