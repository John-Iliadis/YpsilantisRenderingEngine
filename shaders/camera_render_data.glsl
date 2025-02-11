struct CameraRenderData
{
    mat4 view;
    mat4 projection;
    mat4 viewProj;
    vec4 position;
    vec4 direction;
    float nearPlane;
    float farPlane;
};