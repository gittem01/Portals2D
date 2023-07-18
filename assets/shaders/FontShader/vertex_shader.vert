#version 460 core

layout (location = 0) in vec2 vertex;

layout (location = 0) out vec2 texCoord;

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
	texCoord = vec2(vertex.x, -vertex.y) + vec2(0.5, 0.5);

	gl_Position = sharedUboData.orthoMatrix * vec4(vertex * pc.data.zw + pc.data.xy, 0.0, 1.0);
}