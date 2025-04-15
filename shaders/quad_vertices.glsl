
const vec3 quadVertices[4] = vec3[4] (
    vec3(-1.0, 1.0, 1.0), // top left
    vec3(-1.0, -1.0, 1.0), // bottom left
    vec3(1.0, -1.0, 1.0), // bottom right
    vec3(1.0, 1.0, 1.0) // top right
);

const uint quadIndices[6] = uint[6] (
    0, 1, 2,
    0, 2, 3
);
