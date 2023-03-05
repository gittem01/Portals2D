#version 460 core

layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;

layout (location = 0) out vec4 outNormal;
layout (location = 1) out vec4 localPos;

layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 cameraMatrix;
    vec4 cameraPos;
    vec4 data;
} sharedUboData;

layout (std140, set = 1, binding = 0) readonly buffer UBO
{
    mat4 transformMatrix;
    mat4 rotationMatrix;
    vec4 bgColour;
    vec4 dotColour;
    vec4 edgeColour;
    vec4 leftUp;
    vec4 data; // dotSize, sideMargin, sideMult, smoothRatio
    vec4 data2; // sizeX, sizeY, unused, unused
} ubo;

void main()
{
    vec4 scaledVertex = vec4(vertex * vec3(ubo.data2.xy, 1.0), 1.0);
    vec4 pos = sharedUboData.cameraMatrix * ubo.transformMatrix * ubo.rotationMatrix * scaledVertex;

    outNormal = ubo.rotationMatrix * vec4(normal, 1.0);
    localPos = ubo.transformMatrix * scaledVertex - ubo.leftUp;
    gl_Position = pos;
}