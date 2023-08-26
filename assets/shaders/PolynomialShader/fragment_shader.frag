#version 460 core

layout (location = 0) out vec4 FragColour;

layout (location = 0) in vec2 localPos;
layout (location = 1) in vec2 worldPos;
layout (location = 2) flat in vec4 equationData;
layout (location = 3) in float arcPos;


layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 cameraMatrix;
    vec4 cameraPos;
    vec4 data;
} sharedUboData;

float getY(float x)
{
    return equationData.x * pow(x, 2.0) + equationData.y * x + equationData.z;
}

vec2 getDer(float x)
{
    return normalize(vec2(1.0, equationData.x * 2.0 * x + equationData.y));
}

void main()
{
    float distMult = smoothstep(0.0, 0.5 / sharedUboData.data.x, (0.5 - abs(localPos.y)) * 2.0);

    float v = mod(arcPos - sharedUboData.data.y * 0.3, 0.2);
    if (v > 0.15) v = 0.0;
    if (v > 0.075) v = 0.15 - v;

    v = smoothstep(0.0, 0.01 / sharedUboData.data.x, v);


    FragColour = vec4(1.0, 1.0, 1.0, distMult * v);
}