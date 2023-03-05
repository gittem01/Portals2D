#version 460 core

layout (location = 0) in vec2 vertex;

layout (location = 0) out vec2 localPos;
layout (location = 1) out vec2 worldPosition;

layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 orthoMatrix;
    vec4 cameraPos;
    vec4 data; // zoom, unused, unused, unused
} sharedUboData;

layout (std140, set = 1, binding = 0) readonly buffer UBO
{
    mat4 modelMatrix;
    vec4 portals[4];
    vec4 colour;
    vec4 outerColour;
    vec4 data; // unused, lineThickness, outerSmoothThicknessRatio, unused
    vec4 extraData;
} ubo;


void main()
{
    localPos = vertex;
    vec4 wp = ubo.modelMatrix * vec4(vertex, 0.0, 1.0f);
    worldPosition = wp.xy;

    gl_Position = sharedUboData.orthoMatrix * wp;
}