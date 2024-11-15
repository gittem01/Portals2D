#version 460 core

layout (location = 0) in vec2 vertex;

layout (location = 0) out vec2 localPos;
layout (location = 1) out vec2 worldPos;
layout (location = 2) flat out vec2 size;
layout (location = 3) flat out vec4 inColour;
layout (location = 4) flat out vec4 outColour;
layout (location = 5) flat out vec4 outData;
layout (location = 6) flat out int outInstanceIndex;

layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 orthoMatrix;
    vec4 cameraPos;
    vec4 data; // zoom, unused, unused, unused
} sharedUboData;

struct storageData
{
    vec4 posSize;
    vec4 inColour;
    vec4 outColour;
    vec4 data; // squarePattern, outerThick, outerSmooth, innerSmooth
};

layout (std140, set = 1, binding = 0) readonly buffer UBO
{
    storageData sData[];
} ubo;

void main()
{
    outInstanceIndex = gl_InstanceIndex % 64;
    storageData data = ubo.sData[outInstanceIndex];

    size = data.posSize.zw;
    inColour = data.inColour;
    outColour = data.outColour;
    outData = data.data;
    localPos = vertex * size;
    worldPos = localPos + data.posSize.xy;

    gl_Position = sharedUboData.orthoMatrix * vec4(worldPos, 0.0, 1.0);
}