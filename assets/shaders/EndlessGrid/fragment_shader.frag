#version 460 core

#define PI 3.14159265359

layout (location = 0) out vec4 FragColour;

layout (location = 0) in vec2 localPos;
layout (location = 1) flat in vec2 size;
layout (location = 2) in vec2 worldPos;

layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 cameraMatrix;
    vec4 cameraPos;
    vec4 data;
} sharedUboData;

void main()
{
    float lp;
    if (size.x < size.y)
    {
        lp = localPos.x;
    }
    else
    {
        lp = localPos.y;
    }
    float distMult = max((10.0 - pow(length(sharedUboData.data.zw - worldPos), 3.0)) / 7.0, 0.5);
    float m = pow((0.5 - abs(lp)), 3.0) * 2.0;
    FragColour = vec4(m * distMult);
}