#version 460 core

#define PI 3.14159265359

layout (location = 0) in vec2 vertex;

layout (location = 0) out vec2 localPos;
layout (location = 1) flat out vec2 size;
layout (location = 2) out vec2 worldPos;
layout (location = 3) flat out vec4 colour;
layout (location = 4) flat out float minDistMult;

layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 cameraMatrix;
    vec4 cameraPos; // pos.xy, size.xy
    vec4 data;
} sharedUboData;

layout (push_constant) uniform PC
{
    int positionData[4]; // posx, posy, sizex sizey
    vec4 colour;
} pc;

void main()
{
    int index = gl_InstanceIndex;
    int numIndices = pc.positionData[2];
    vec2 pos = vec2(pc.positionData[0], pc.positionData[1]) + 0.5;

    int indexX = index % numIndices;
    int indexY = index / numIndices;

    pos += vec2(float(indexX), float(indexY));

    localPos = vertex;
    worldPos = vertex + pos;
    colour = pc.colour;
    if ((indexX != numIndices / 2 || indexY != numIndices / 2) && pc.positionData[3] == 0)
    {
        colour.w = 0.1;
        minDistMult = 0.0;
    }
    else
    {
        minDistMult = 1.0;
    }

    vec4 outPos = sharedUboData.cameraMatrix * vec4(worldPos, 0.0, 1.0);

    gl_Position = outPos;
}