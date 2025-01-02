#version 460 core

#define PI 3.141592653589793238

layout (location = 0) out vec2 localPos;
layout (location = 1) flat out vec2 size;

layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 orthoMatrix;
    vec4 cameraPos;
    vec4 data; // zoom, unused, unused, unused
} sharedUboData;


layout (push_constant) uniform PC
{
    vec4 positions;
    vec4 colour;
    vec4 data; // unused, lineThickness, outerSmoothThicknessRatio, unused
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

vec2 vertices[6] =
{
    vec2(-0.5f, +0.5f),
    vec2(+0.5f, +0.5f),
    vec2(+0.5f, -0.5f),
    vec2(+0.5f, -0.5f),
    vec2(-0.5f, -0.5f),
    vec2(-0.5f, +0.5f),
};

void main()
{
    float zoom = sharedUboData.data.x;
    vec2 diffVec = pc.positions.zw - pc.positions.xy;
    vec2 midPoint = (pc.positions.xy + pc.positions.zw) * 0.5f;
    float dist = length(diffVec);
    float angle;

    vec2 vertex = vertices[gl_VertexIndex];
    // scale, rotate, transform
    vec2 vCopy = vertex;
    // scale
    vCopy.x *= dist;
    vCopy.y *= pc.data.y / (zoom * zoom);

    // rotate
    if (diffVec.x == 0)
    {
        angle = PI / 2;
    }
    else
    {
        angle = atan(diffVec.y / diffVec.x);
    }

    float ySize = (pc.data.y / (zoom * zoom)) * 0.5f;
    float xSize = dist * 0.5f;
    size = vec2(xSize + ySize, ySize);
    if (vertex.x > 0)
    {
        vCopy.x += ySize;
    }
    else
    {
        vCopy.x -= ySize;
    }

    localPos = vCopy;
    vCopy = rotateAroundOrigin(vCopy, angle);
    // transform
    vCopy += midPoint;

    gl_Position = sharedUboData.orthoMatrix * vec4(vCopy, 0.0, 1.0);
}