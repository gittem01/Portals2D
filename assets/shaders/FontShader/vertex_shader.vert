#version 460 core

layout (location = 0) in vec2 vertex;

layout (location = 0) out vec2 texCoord;
layout (location = 1) flat out vec4 outData;

layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 orthoMatrix;
    vec4 cameraPos;
    vec4 data; // zoom, unused, unused, unused
} sharedUboData;

layout (push_constant) uniform PC
{
    vec4 data;
} pc;

void main() 
{
    const float scaleUp = 1.5; // scale vertices but keep the look same
	texCoord = (vec2(vertex.x, -vertex.y) * scaleUp + vec2(0.5, 0.5));
    outData = pc.data;

	gl_Position = sharedUboData.orthoMatrix * vec4(vertex * pc.data.zw * scaleUp + pc.data.xy, 0.0, 1.0);
}