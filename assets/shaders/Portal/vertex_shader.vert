#version 460 core

layout (location = 0) in vec2 vertex;

layout (location = 0) out vec2 localPos;
layout (location = 1) out vec2 realLocalPos;

layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 cameraMatrix;
    vec4 cameraPos;
    vec4 data;
} sharedUboData;

layout (std140, set = 1, binding = 0) uniform UBO
{
    mat4 modelMatrix;
    vec4 data0; // a, b, abRatio, eThick
    vec4 data1; // lineLength, maxAngleInc, minAngleInc, noiseScale
    vec4 data2; // sizex, sizey, iTime, 0
    vec4 data3; // funcMult0, funcMult1, 0, 0
    vec4 portalColour;
    vec4 edgeColour;
} ubo;

void main()
{
    localPos = vertex * ubo.data2.xy;
    realLocalPos = (vertex + 0.5) * 0.5;

    gl_Position = sharedUboData.cameraMatrix * ubo.modelMatrix * vec4(0.0, localPos.y, localPos.x, 1.0);
}