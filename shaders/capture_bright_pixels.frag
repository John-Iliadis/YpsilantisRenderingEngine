#version 460 core

layout (location = 0) in vec2 vTexCoords;

layout (location = 0) out vec4 outFragColor;

layout (push_constant) uniform PushConstants
{
    float brightnessThreshold;
};

layout (set = 0, binding = 0) uniform sampler2D tex;

void main()
{
    vec4 sampleColor = texture(tex, vTexCoords);
    float brightness = dot(sampleColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    outFragColor = sampleColor * float(brightness >= brightnessThreshold);
    outFragColor.a = 1.0;
}
