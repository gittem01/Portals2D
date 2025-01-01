#version 460 core

layout (location = 0) in vec2 vertex;

layout (location = 0) out vec2 localPos;
layout (location = 1) out vec2 worldPos;
layout (location = 2) flat out vec4 lineColour;
layout (location = 3) flat out vec4 bgColour;
layout (location = 4) flat out vec4 sizeLtRat;
layout (location = 5) flat out float gridSize;
layout (location = 6) flat out int sideData;

layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 cameraMatrix;
    vec4 cameraPos;
    vec4 data;
} sharedUboData;

struct storageData
{
    vec4 posSize;
    vec4 colour;
    vec4 bgColour;
    vec4 data; // gridSize, lineThick, camMult, gridRatio
    ivec4 sideData; // side, unused, unused, unused
};

layout (std140, set = 1, binding = 0) readonly buffer UBO
{
    storageData sData[];
} ubo;

void main()
{
    int instanceIndex = gl_InstanceIndex;

    storageData data = ubo.sData[instanceIndex];
    localPos = vertex * data.posSize.zw;
    worldPos = localPos + data.posSize.xy;
    lineColour = data.colour;
    bgColour = data.bgColour;
    sizeLtRat = vec4(data.posSize.zw, data.data.y, data.data.w);
    gridSize = data.data.x;
    sideData = data.sideData.x;

    mat4 newCameraMatrix = sharedUboData.cameraMatrix;

    newCameraMatrix[3][0] *= data.data.z;
    newCameraMatrix[3][1] *= data.data.z;

    gl_Position = newCameraMatrix * vec4(worldPos, 0.0, 1.0);
}