#version 460 core

layout (triangles) in;
layout (triangle_strip) out;
layout (max_vertices = 3) out;

layout (location = 0) noperspective out vec3 vEdgeDistance;

layout (push_constant) uniform PushConstants
{
    mat4 viewportMat;
};

// P is the point
// L = A + tV is the line equation, where A is a point on the line and V is a vector along the line
float pointToLineDistance(vec2 P, vec2 A, vec2 V)
{
    vec2 AP = P - A;
    return length(cross(vec3(AP, 0.0), vec3(V, 0.0))) / length(V);
}

void main()
{
    // calculate the triangle points in screen space
    vec2 P0 = vec2(viewportMat * (gl_in[0].gl_Position / gl_in[0].gl_Position.w));
    vec2 P1 = vec2(viewportMat * (gl_in[1].gl_Position / gl_in[1].gl_Position.w));
    vec2 P2 = vec2(viewportMat * (gl_in[2].gl_Position / gl_in[2].gl_Position.w));

    float h0 = pointToLineDistance(P2, P0, P1 - P0);
    float h1 = pointToLineDistance(P0, P1, P2 - P1);
    float h2 = pointToLineDistance(P1, P0, P2 - P0);

    // vertex 1
    gl_Position = gl_in[0].gl_Position;
    vEdgeDistance = vec3(0, h1, 0);
    EmitVertex();

    // vertex 2
    gl_Position = gl_in[1].gl_Position;
    vEdgeDistance = vec3(0, 0, h2);
    EmitVertex();

    // vertex 3
    gl_Position = gl_in[2].gl_Position;
    vEdgeDistance = vec3(h0, 0, 0);
    EmitVertex();

    EndPrimitive();
}
