#version 460 core

#define PI 3.141592653589793238

layout (location = 0) in vec2 vertex;

layout (location = 0) out vec2 localPos;
layout (location = 1) out vec2 worldPos;
layout (location = 2) flat out vec2 size;
layout (location = 3) flat out int instanceIndex;

layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 orthoMatrix;
    vec4 cameraPos;
    vec4 data; // zoom, unused, unused, unused
} sharedUboData;

struct storageData
{
    vec4 pointts;
    vec4 colour;
    vec4 data; // thickness, none, none, none
};

layout (std140, set = 1, binding = 0) readonly buffer UBO
{
    storageData sData[];
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
    storageData data = ubo.sData[instanceIndex];

    vec2 diffVec = data.pointts.zw - data.pointts.xy;
    vec2 midPoint = (data.pointts.xy + data.pointts.zw) * 0.5f;
    float dist = length(diffVec);
    float angle;

    // scale, rotate, transform
    vec2 vCopy = vertex;
    // scale
    vCopy.x *= dist;
    vCopy.y *= data.data.x;

    // rotate
    if (diffVec.x == 0){
        angle = PI / 2;
    }
    else{
        angle = atan(diffVec.y / diffVec.x);
    }

    size = vec2(dist * 0.5f + data.data.x * 0.5, data.data.x * 0.5f);
    if (vertex.x > 0)
    {
        vCopy.x += size.y;
    }
    else
    {
        vCopy.x -= size.y;
    }

    localPos = vCopy;

    vCopy = rotateAroundOrigin(vCopy, angle);
    vCopy += midPoint;

    worldPos = vCopy;

    gl_Position = sharedUboData.orthoMatrix * vec4(vCopy, 0.0, 1.0);
}