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
    vec4 data;
} pc;

vec2 rotateAroundOrigin(vec2 point, float angle)
{
    float s = sin(angle);
    float c = cos(angle);

    vec2 p;

    p.x = point.x * c - point.y * s;
    p.y = point.y * c + point.x * s;

    return p;
}

void main() 
{
    localPos = vertex * pc.posSize.zw;
    vec2 altVec = localPos + pc.posSize.xy;
	gl_Position = sharedUboData.orthoMatrix * vec4(altVec, 0.0, 1.0);
}