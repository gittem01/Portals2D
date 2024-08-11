#version 460 core

layout (location = 0) out vec2 localPosition;
layout (location = 1) flat out ivec4 outData;
layout (location = 2) flat out float angle;
layout (location = 3) out vec2 wp;

layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 orthoMatrix;
    vec4 cameraPos;
    vec4 data; // zoom, unused, unused, unused
} sharedUboData;

struct perGelLineData
{
    vec4 positions;
    ivec4 data; // size, gelDataOffset, prevOffset, nextOffset
};

struct pergGelTickData
{
    vec4 data; // colorIndex, gelHeight, time, force
};

layout (std140, set = 1, binding = 0) readonly buffer UBO
{
    perGelLineData sData[];
} ubo;

layout (std140, set = 1, binding = 1) readonly buffer UBO2
{
    pergGelTickData sData[];
} ubo2;

vec2 vertexArray[6] = 
{
    vec2(0.0, 0.0),
    vec2(1.0, 0.0),
    vec2(0.0, 1.0),
    vec2(1.0, 0.0),
    vec2(1.0, 1.0),
    vec2(0.0, 1.0)
};

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
    perGelLineData data = ubo.sData[gl_InstanceIndex];
    outData = data.data;
    vec2 p0 = data.positions.xy;
    vec2 p1 = data.positions.zw;
    angle = atan(p1.y - p0.y, p1.x - p0.x);
    vec2 p = vertexArray[gl_VertexIndex];
    p.x *= length(p0 - p1);
    localPosition = p;
    p.y *= -0.4;
    wp = rotateAroundOrigin(p, angle) + p0;

	gl_Position = sharedUboData.orthoMatrix * vec4(wp, 0.0, 1.0);
}