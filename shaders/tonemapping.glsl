
vec3 applyExposure(vec3 color, float exposure)
{
    return color * exposure;
}

vec3 reinhard(vec3 color)
{
    return color / (color + vec3(1.0));
}

vec3 reinhardExtended(vec3 color, float maxWhite)
{
    vec3 numerator = color * (1.0 + color / (maxWhite * maxWhite));
    return numerator / (1.0 + color);
}

vec3 narkowiczAces(vec3 color)
{
    const float a = 2.51f;
    const float b = 0.03f;
    const float c = 2.43f;
    const float d = 0.59f;
    const float e = 0.14f;

    return clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
}

vec3 RRTandODTFit(vec3 color)
{
    vec3 a = color * (color + 0.0245786f) - 0.000090537f;
    vec3 b = color * (0.983729f * color + 0.4329510f) + 0.238081f;

    return a / b;
}

vec3 hillAces(vec3 color)
{
    const mat3 inputMat = mat3(
        0.59719, 0.35458, 0.04823,
        0.07600, 0.90834, 0.01566,
        0.02840, 0.13383, 0.8377
    );

    const mat3 outputMat = mat3(
        1.60475, -0.53108, -0.07367,
        -0.10208,  1.10813, -0.00605,
        -0.00327, -0.07276,  1.07602
    );

    color = inputMat * color;
    color = RRTandODTFit(color);
    color = outputMat * color;

    return clamp(color, 0.0, 1.0);
}
