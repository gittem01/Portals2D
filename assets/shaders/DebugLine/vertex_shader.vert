#version 460 core

#define PI 3.141592653589793238

layout (location = 0) in vec2 vertex;

layout (location = 0) out vec2 localPos;
layout (location = 1) out vec2 size;
layout (location = 2) flat out int instanceIndex;

layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 orthoMatrix;
    vec4 cameraPos;
    vec4 data; // zoom, unused, unused, unused
} sharedUboData;

struct lineData
{
    vec4 positions;
    vec4 colour;
    vec4 data; // unused, lineThickness, outerSmoothThicknessRatio, unused
};

layout (std140, set = 1, binding = 0) readonly buffer UBO
{
    lineData sData[];
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
    instanceIndex = gl_InstanceIndex;
    lineData data = ubo.sData[instanceIndex];

    float zoom = sharedUboData.data.x;
    vec2 diffVec = data.positions.zw - data.positions.xy;
    vec2 midPoint = (data.positions.xy + data.positions.zw) * 0.5f;
    float dist = length(diffVec);
    float angle;

    // scale, rotate, transform
    vec2 vCopy = vertex;
    // scale
    vCopy.x *= dist;
    vCopy.y *= data.data.y / (zoom * zoom);

    // rotate
    if (diffVec.x == 0){
        angle = PI / 2;
    }
    else{
        angle = atan(diffVec.y / diffVec.x);
    }

    float ySize = (data.data.y / (zoom * zoom)) * 0.5f;
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