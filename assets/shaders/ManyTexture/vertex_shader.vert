#version 460 core

layout (location = 0) in vec2 vertex;

layout (location = 0) out vec4 localPos;
layout (location = 1) out vec4 worldPos;
layout (location = 2) out vec2 out_tc;

layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 cameraMatrix;
    vec4 cameraPos;
    vec4 data;
} sharedUboData;

struct storageData
{
    vec4 posSize;
    vec4 data;
};

layout (std140, set = 1, binding = 0) readonly buffer UBO
{
    storageData sData[];
} ubo;

void main()
{
    int instanceID = gl_InstanceIndex;

    mat4 newCameraMatrix = sharedUboData.cameraMatrix;
//    newCameraMatrix[3][0] *= data2.z;
//    newCameraMatrix[3][1] *= data2.z;

    vec2 scaledVertex = vertex * ubo.sData[instanceID].posSize.zw;
    worldPos = vec4(ubo.sData[instanceID].posSize.xy + scaledVertex, 0.0, 1.0);
    vec4 pos = newCameraMatrix * worldPos;

    out_tc = (vertex + 0.5);

    gl_Position = pos;
}