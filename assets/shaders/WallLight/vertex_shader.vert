#version 460 core

layout (location = 0) in vec2 vertex;

layout (location = 0) out vec2 localPos;
layout (location = 1) out vec2 worldPos;
layout (location = 2) out flat int instanceIndex;

layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 cameraMatrix;
    vec4 cameraPos;
    vec4 data;
} sharedUboData;

struct uboData
{
    vec4 posSize;
    vec4 color;
    vec4 data; // lightXSize, lightYSize, upDownSize, lightDepth
    vec4 data2; // rotation, glassRotation (added on top of rotation), camMult, lightScatterRatio
};

layout (set = 1, binding = 0) readonly buffer UBO
{
    uboData data[];
} ubo;

vec2 rotatePoint(vec2 point, float angle)
{
    float x = cos(angle) * point.x - sin(angle) * point.y;
    float y = sin(angle) * point.x + cos(angle) * point.y;

    return vec2(x, y);
}

void main()
{
    instanceIndex = gl_InstanceIndex;
    localPos = vertex * ubo.data[instanceIndex].posSize.zw;
    vec4 posSize = ubo.data[instanceIndex].posSize;
    worldPos = rotatePoint(vertex * posSize.zw, ubo.data[instanceIndex].data2.x) + posSize.xy;

    mat4 newCameraMatrix = sharedUboData.cameraMatrix;
    newCameraMatrix[3][0] *= ubo.data[instanceIndex].data2.z;
    newCameraMatrix[3][1] *= ubo.data[instanceIndex].data2.z;

    gl_Position = newCameraMatrix * vec4(worldPos, 0.0, 1.0);
}