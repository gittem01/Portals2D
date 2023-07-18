#version 460 core

#define PI 3.14159265359

layout (location = 0) out vec4 FragColour;

layout (location = 0) in vec2 localPos;
layout (location = 1) flat in vec2 size;
layout (location = 2) in vec2 worldPos;
layout (location = 3) flat in vec4 colour;
layout (location = 4) flat in float minDistMult;

layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 cameraMatrix;
    vec4 cameraPos;
    vec4 data;
} sharedUboData;

void main()
{
    float zoom = sharedUboData.data.x;
    float zoomSqr = zoom * zoom;

    float distMult = max((2.0 - pow(length(sharedUboData.data.zw - worldPos), 1.0)), minDistMult);
    float m = (pow(abs(localPos.x), 10.0) + pow(abs(localPos.y), 10.0));
    FragColour = colour * m * 750.0 * distMult;
}