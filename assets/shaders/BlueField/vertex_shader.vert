#version 460 core

#define PI 3.141592653589793238

layout (location = 0) in vec2 vertex;

layout (location = 0) out vec2 localPos;
layout (location = 1) out vec2 worldPos;
layout (location = 2) flat out vec2 size;
layout (location = 3) flat out vec4 colour;
layout (location = 4) flat out vec4 colour2;
layout (location = 5) flat out vec4 outData;
layout (location = 6) flat out vec4 shootData;

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
    vec4 colour2;
    vec4 data; // thickness, timeOffset, status, statusChangeTime
    vec4 shootData; // shootPos.x, shootPos.y, shootTime, unused
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
    int instanceIndex = gl_InstanceIndex;
    storageData data = ubo.sData[instanceIndex];

    colour = data.colour;
    colour2 = data.colour2;
    outData = data.data;
    shootData = data.shootData;

    vec2 diffVec = data.pointts.zw - data.pointts.xy;
    float dist = length(diffVec);
    float angle;

    // scale, rotate, transform
    vec2 vCopy = vertex;
    // scale
    vCopy.x *= dist;
    vCopy.y *= data.data.x;

    const float smoothSize = min(max(dist * 0.1, 0.3), 5.0);
    float timeDiff = (sharedUboData.data.y - data.data.w) * min(200.0, dist * 5.0);
    float splitX = (-smoothSize + timeDiff) * data.data.z + (dist * 0.5 + smoothSize - timeDiff) * (1.0 - data.data.z);
    if (splitX < -smoothSize)
    {
        // discard instance
        gl_Position = vec4(-10.0, -10.0, 0.0, 1.0);
        return;
    }

    // rotate
    if (diffVec.x == 0)
    {
        angle = diffVec.y > 0.0 ? PI * 0.5 : -PI * 0.5;
    }
    else
    {
        angle = atan(diffVec.y, diffVec.x);
    }

    size = vec2(dist, data.data.x);

    localPos = vec2(vCopy.x, vertex.y * size.y);

    vec2 midPoint = (data.pointts.xy + data.pointts.zw) * 0.5f;
    vCopy = rotateAroundOrigin(vCopy, angle);
    vCopy += midPoint;

    worldPos = vCopy;

    gl_Position = sharedUboData.orthoMatrix * vec4(vCopy, 0.0, 1.0);
}