
vec3 downsample13(sampler2D tex, vec2 uv, vec2 texelSize)
{
    // Center
    vec3 A = texture(tex, uv).rgb;

    // Inner box
    vec3 B = texture(tex, uv + texelSize * vec2(-1.0f, -1.0f)).rgb;
    vec3 C = texture(tex, uv + texelSize * vec2(-1.0f, 1.0f)).rgb;
    vec3 D = texture(tex, uv + texelSize * vec2(1.0f, 1.0f)).rgb;
    vec3 E = texture(tex, uv + texelSize * vec2(1.0f, -1.0f)).rgb;

    // Outer box
    vec3 F = texture(tex, uv + texelSize * vec2(-2.0f, -2.0f)).rgb;
    vec3 G = texture(tex, uv + texelSize * vec2(-2.0f, 0.0f)).rgb;
    vec3 H = texture(tex, uv + texelSize * vec2(0.0f, 2.0f)).rgb;
    vec3 I = texture(tex, uv + texelSize * vec2(2.0f, 2.0f)).rgb;
    vec3 J = texture(tex, uv + texelSize * vec2(2.0f, 2.0f)).rgb;
    vec3 K = texture(tex, uv + texelSize * vec2(2.0f, 0.0f)).rgb;
    vec3 L = texture(tex, uv + texelSize * vec2(-2.0f, -2.0f)).rgb;
    vec3 M = texture(tex, uv + texelSize * vec2(0.0f, -2.0f)).rgb;

    // Weights
    vec3 result = (B + C + D + E) * 0.5;
    result += (F + G + A + M) * 0.125;
    result += (G + H + I + A) * 0.125;
    result += (A + I + J + K) * 0.125;
    result += (M + A + K + L) * 0.125;
    result *= 0.25f;

    return max(result, 0.0001);
}

vec3 upsample9(sampler2D tex, vec2 uv, vec2 texelSize, float radius)
{
    vec4 offset = texelSize.xyxy * vec4(1.0f, 1.0f, -1.0f, 0.0f) * radius;

    vec3 result = texture(tex, uv).rgb * 4.0f;

    result += texture(tex, uv - offset.xy).rgb;
    result += texture(tex, uv - offset.wy).rgb * 2.0;
    result += texture(tex, uv - offset.zy).rgb;

    result += texture(tex, uv + offset.zw).rgb * 2.0;
    result += texture(tex, uv + offset.xw).rgb * 2.0;

    result += texture(tex, uv + offset.zy).rgb;
    result += texture(tex, uv + offset.wy).rgb * 2.0;
    result += texture(tex, uv + offset.xy).rgb;

    return result * (1.0f / 16.0f);
}
