#version 460 core

#define PI 3.141592653589793238

layout (location = 0) in vec2 vertex;

layout (location = 0) out vec2 localPos;
layout (location = 1) flat out vec2 size;
layout (location = 2) flat out vec4 colour;
layout (location = 3) flat out vec4 portal;
layout (location = 4) out vec2 worldPos;

layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 orthoMatrix;
    vec4 cameraPos;
    vec4 data; // zoom, unused, unused, unused
} sharedUboData;

layout (push_constant) uniform UBO
{
    vec4 positions;
    vec4 colour;
    vec4 portal;
    vec4 data; // ySize, timeMult, none, none
} ubo;

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
    vec2 p1 = ubo.positions.xy;
    vec2 p2 = ubo.positions.zw;
    vec2 diff = p2 - p1;

    size = vec2(length(p2 - p1), ubo.data.x) * 20.0f;
    localPos = (vertex + 0.5) * size;
    colour = ubo.colour;
    portal = ubo.portal;

    float rot = atan(diff.y, diff.x);
    vec2 mid = p1 + diff * 0.5;
    worldPos = rotateAroundOrigin(vertex * size * 0.05, rot) + mid;
    gl_Position = sharedUboData.orthoMatrix * vec4(worldPos, 0.0, 1.0);
}