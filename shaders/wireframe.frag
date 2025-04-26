#version 460 core

layout (early_fragment_tests) in;

layout (location = 0) noperspective in vec3 vEdgeDistance;

layout (location = 0) out vec4 outFragColor;

layout (push_constant) uniform PushConstants
{
    layout (offset = 64) vec4 wireframeColor;
    float wireframeWidth;
};

void main()
{
    float minEdgeDistance = min(vEdgeDistance.x, min(vEdgeDistance.y, vEdgeDistance.z));
    float wireframeHalfWidth = wireframeWidth * 0.5;

    float alpha = 0.0;
    if (minEdgeDistance < wireframeHalfWidth - 1.0)
    {
        alpha = 1.0;
    }
    else if (minEdgeDistance > wireframeHalfWidth + 1.0)
    {
        alpha = 0.0;
    }
    else
    {
        float x = minEdgeDistance - (wireframeHalfWidth - 1.0);
        alpha = exp2(-2.0 * pow(x, 3.0));
    }

    outFragColor = wireframeColor;
    outFragColor.a = alpha;
}
