#version 460 core

layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 tc;

layout (location = 0) out vec4 outNormal;
layout (location = 1) out vec4 worldPos;
layout (location = 2) out vec2 out_tc;


layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 cameraMatrix;
    vec4 cameraPos;
    vec4 data;
} sharedUboData;

layout (std140, set = 1, binding = 0) uniform UBO
{
    mat4 modelMatrix;
    mat4 rotationMatrix;
} ubo;

void main()
{
    out_tc = tc;
    outNormal = ubo.rotationMatrix * vec4(normal, 1.0);
    worldPos = ubo.modelMatrix * vec4(vertex, 1.0);
    gl_Position = sharedUboData.cameraMatrix * worldPos;
}