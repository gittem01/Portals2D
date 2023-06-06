#version 460 core

layout (location = 0) in vec2 vertex;

layout (location = 0) out vec2 localPos;
layout (location = 1) out vec2 worldPos;
layout (location = 2) flat out int instanceIndex;


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
    vec4 data; // isVertical, lineSize, none, none
};

layout (std140, set = 1, binding = 0) readonly buffer UBO
{
    storageData sData[];
} ubo;

void main()
{
    instanceIndex = gl_InstanceIndex;

    storageData data = ubo.sData[instanceIndex];
    localPos = vertex * data.posSize.zw;
    worldPos = localPos + data.posSize.xy;

    mat4 newCameraMatrix = sharedUboData.cameraMatrix;

    newCameraMatrix[3][0] *= 0.8;
    newCameraMatrix[3][1] *= 0.8;

    gl_Position = newCameraMatrix * vec4(worldPos, 0.0, 1.0);
}