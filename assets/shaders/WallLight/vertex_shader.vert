#version 460 core

layout (location = 0) in vec2 vertex;

layout (location = 0) out vec2 localPos;
layout (location = 1) out vec2 worldPos;

layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 cameraMatrix;
    vec4 cameraPos;
    vec4 data;
} sharedUboData;

layout (set = 1, binding = 0) uniform UBO
{
    vec4 posSize;
    vec4 color;
    vec4 data; // lightXSize, lightYSize, upDownSize, lightDepth
    vec4 data2; // rotation, none, none, none
} ubo;

vec2 rotatePoint(vec2 point, float angle)
{
    float x = cos(angle) * point.x - sin(angle) * point.y;
    float y = sin(angle) * point.x + cos(angle) * point.y;

    return vec2(x, y);
}

void main()
{
    localPos = vertex * ubo.posSize.zw;
    worldPos = rotatePoint(vertex * ubo.posSize.zw, ubo.data2.x) + ubo.posSize.xy;
    gl_Position = sharedUboData.cameraMatrix * vec4(worldPos, 0.0, 1.0);
}