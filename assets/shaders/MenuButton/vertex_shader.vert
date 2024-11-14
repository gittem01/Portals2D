#version 460 core

layout (location = 0) out vec2 localPos;
layout (location = 1) out vec2 worldPos;

layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 orthoMatrix;
    vec4 cameraPos;
    vec4 data; // zoom, unused, unused, unused
} sharedUboData;

layout (push_constant) uniform PC
{
    vec4 posSize;
    vec4 colour;
    vec4 data; // extraFill, smoothFill, unused, unused
} pc;

vec2 vertices[6] =
{
    vec2(-0.5f, +0.5f),
    vec2(+0.5f, +0.5f),
    vec2(+0.5f, -0.5f),
    vec2(+0.5f, -0.5f),
    vec2(-0.5f, -0.5f),
    vec2(-0.5f, +0.5f),
};

void main()
{
    vec2 vertex = vertices[gl_VertexIndex];
    // scale, rotate, transform
    vec2 vCopy = vertex;
    // scale
    vCopy *= pc.posSize.zw + pc.data.y;

    localPos = vCopy;

    vCopy += pc.posSize.xy;

    worldPos = vCopy;

    gl_Position = sharedUboData.orthoMatrix * vec4(vCopy, 0.0, 1.0);
}