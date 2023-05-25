#version 460 core

#define PI 3.14159265359
#define SPEED 0.03

layout (location = 0) out vec4 FragColour;

layout (location = 0) in vec2 localPos;

layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 cameraMatrix;
    vec4 cameraPos;
    vec4 data;
} sharedUboData;

layout (std140, set = 1, binding = 0) uniform UBO
{
    vec4 portalPoints;
    vec4 color;
    vec4 data0; // dist, speed, time, angle
    vec4 data1; // sizex, sizey, randomization, none
} ubo;

float editorFunc(float val)
{
    float lastMult;
    if (val < 0.0) lastMult = -1.0f;
    else lastMult = 1.0f;
    return pow(val, 10.0);
}

void main()
{
    float pixeller = 0.05;
    vec2 localPosCopy = localPos;
    localPosCopy.x = int(localPos.x / pixeller) * pixeller;
    localPosCopy.y = int(localPos.y / pixeller) * pixeller;

    float len = length(localPosCopy);
    if (len > 0.5)
    {
        FragColour = vec4(0.0);
    }
    else
    {
        float val = 1.0 - len * 2.0;
        FragColour = vec4(ubo.color.xyz, editorFunc(val));
    }
}