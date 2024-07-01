#version 460 core

layout (location = 0) out vec2 localPosition;
layout (location = 1) out vec2 worldPosition;

layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 orthoMatrix;
    vec4 cameraPos;
    vec4 data; // zoom, unused, unused, unused
} sharedUboData;

layout (push_constant) uniform PC
{
	vec4 positions;
    vec4 data; // thickness, unused, unused, unused
} pc;

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
    vec2 p0 = vec2(-4.0, 0.0);
    vec2 p1 = vec2(+4.0, 0.0);
    float angle = atan(p1.y - p0.y, p1.x - p0.x);
    vec2 p = vertexArray[gl_VertexIndex];
    p.x *= length(p0 - p1);
    localPosition = p;
    p.y *= -0.5;
    p = rotateAroundOrigin(p, angle) + p0;
    worldPosition = p;
    vec4 ppp = sharedUboData.orthoMatrix * vec4(p, 0.0, 1.0);
	gl_Position = ppp;
}