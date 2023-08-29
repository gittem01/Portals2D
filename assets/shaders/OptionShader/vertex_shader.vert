#version 460 core

layout (location = 0) in vec2 vertex;

layout (location = 0) out vec2 localPos;

layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 orthoMatrix;
    vec4 cameraPos;
    vec4 data; // zoom, unused, unused, unused
} sharedUboData;

layout (push_constant) uniform PC
{
    vec4 posSize;
    ivec4 data;
} pc;

void main() 
{
    localPos = vertex * pc.posSize.zw;

	gl_Position = sharedUboData.orthoMatrix * vec4(vertex * pc.posSize.zw + pc.posSize.xy + vec2(pc.posSize.z, -pc.posSize.w) * 0.5, 0.0, 1.0);
}