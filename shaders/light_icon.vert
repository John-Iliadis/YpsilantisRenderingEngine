#version 460 core

layout (location = 0) out vec2 vTexCoords;

layout (push_constant) uniform PushConstants { vec3 position; };

layout (set = 0, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 projection;
    mat4 viewProj;
    vec4 cameraPos;
    vec4 cameraDir;
    float nearPlane;
    float farPlane;
};

const float quadHalfWidth = 0.1;

const vec3 quadCoords[4] = vec3[4](
    vec3(-1.0, 1.0, 1.0),
    vec3(1.0, 1.0, 1.0),
    vec3(1.0, -1.0, 1.0),
    vec3(-1.0, -1.0, 1.0)
);

const uint indices[6] = uint[6](0, 1, 2, 0, 2, 3);

void main()
{
    uint index = indices[gl_VertexIndex];
    vec3 quadCoord = quadCoords[index];

    vTexCoords = quadCoord.xy * 0.5 + 0.5;

    vec3 camRight = -normalize(cross(cameraDir.xyz, vec3(0.f, 1.f, 0.f)));
    vec3 camUp = normalize(cross(camRight, -cameraDir.xyz));

    vec3 pos = position;
    pos += camRight * quadCoord.x * quadHalfWidth;
    pos += camUp * quadCoord.y * quadHalfWidth;

    gl_Position = viewProj * vec4(pos, 1.0);
}
