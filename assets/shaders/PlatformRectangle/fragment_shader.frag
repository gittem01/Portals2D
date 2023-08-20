#version 460 core

#define PI 3.14159265359

layout (location = 0) out vec4 FragColour;

layout (location = 0) in vec2 localPos;
layout (location = 1) flat in vec2 size;
layout (location = 2) flat in vec4 inColour;
layout (location = 3) flat in vec4 outColour;
layout (location = 4) flat in vec4 outData;

layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 orthoMatrix;
    vec4 cameraPos;
    vec4 data; // zoom, unused, unused, unused
} sharedUboData;

void main()
{
    float sv = outData.x;
    float sider = outData.y;

    vec4 bgColor = vec4(0.0, 0.0, 0.0, 0.0);
    
    float xDist = size.x * 0.5 - abs(localPos.x);
    float yDist = size.y * 0.5 - abs(localPos.y);
    float dist = min(xDist, yDist);
    
    float n = 0.0;

    if (sv > 0.0)
    {
        n += abs(mod(abs(localPos.x), sv) - sv * 0.5);
        n += abs(mod(abs(localPos.y), sv) - sv * 0.5);
    }

    if (dist < sider * 0.5)
    {
        float v = smoothstep(0.0, 0.4, dist / (sider * 0.5));
        FragColour = outColour * v + (1.0 - v) * bgColor + n;
    }
    else if (dist < sider)
    {
        float v = smoothstep(0.0, 1.0, 1.0 - (dist - sider * 0.5) / (sider * 0.5));
        FragColour = outColour * v + (1.0 - v) * inColour + n;
    }
    else
    {
        FragColour = inColour + n;
    }
}