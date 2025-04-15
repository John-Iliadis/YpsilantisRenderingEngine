#version 460 core

layout (triangles) in;
layout (triangle_strip, max_vertices = 18) out;

layout (location = 0) out vec3 vTexCoords;

layout (set = 0, binding = 0) uniform UBO {
    mat4 proj;
    mat4 views[6];
};

void main()
{
    for (int layer = 0; layer < 6; ++layer)
    {
        gl_Layer = layer;

        for (int i = 0; i < 3; ++i)
        {
            vTexCoords = vec3(views[layer] * gl_in[i].gl_Position);
            gl_Position = gl_in[i].gl_Position;
            EmitVertex();
        }

        EndPrimitive();
    }
}
