#version 460 core

#define PI 3.14159265359

layout (location = 0) in vec2 vertex;

layout (location = 0) out vec2 localPos;

layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 cameraMatrix;
    vec4 cameraPos;
    vec4 data;
} sharedUboData;

layout (push_constant) uniform PC
{
    vec4 posSize; // pos.x, pos.y, size.x, size.y
    vec4 color;
    vec4 data0; // dirAngle, pixeler, midThick, xDropSize
} pc;

vec2 rotatePoint(vec2 point, float angle)
{
    float x = cos(angle) * point.x - sin(angle) * point.y;
    float y = sin(angle) * point.x + cos(angle) * point.y;

    return vec2(x, y);
}

void main()
{
    localPos = vertex * pc.posSize.zw;
    vec2 newVertexPos = rotatePoint(localPos, pc.data0.x - PI * 0.5) + pc.posSize.xy;
    
    gl_Position = sharedUboData.cameraMatrix * vec4(newVertexPos, 0.0, 1.0);
}