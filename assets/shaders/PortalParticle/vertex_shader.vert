#version 460 core

#define PI 3.14159265359

layout (location = 0) in vec2 vertex;

layout (location = 0) out vec2 localPos;
layout (location = 1) out vec4 colour;
layout (location = 2) out vec2 worldPos;

layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 cameraMatrix;
    vec4 cameraPos;
    vec4 data;
} sharedUboData;

layout (std140, set = 1, binding = 0) uniform UBO
{
    vec4 portalPoints;
    vec4 color;
    vec4 data0; // dist, speed, time, upSize
    vec4 data1; // sizex, sizey, randomization, instanceCount
} ubo;

vec2 rotatePoint(vec2 point, float angle)
{
    float c = cos(angle);
    float s = sin(angle);
    mat2 rotationMatrix = mat2(c, -s, s, c);
    return rotationMatrix * point;
}

vec2 rotate90(vec2 v)
{
    return vec2(-v.y, v.x);
}

float randRange(vec2 seed)
{
    float random = fract(sin(dot(seed, vec2(13.8695 + ubo.data1.z, 17.1371))) * 73714.38361);
    
    return random;
}

void main()
{
    float timeToEnd = 1.0 / ubo.data0.y;
    float instanceRatio = (gl_InstanceIndex / ubo.data1.w) * timeToEnd;
    float instanceSpecificRand = randRange(vec2(gl_InstanceIndex, gl_InstanceIndex));
    float time = (sharedUboData.data.y - ubo.data0.z) - instanceRatio;
    float loopTime = fract(time * (ubo.data0.y + instanceSpecificRand));
    int loopInt = int(time * (ubo.data0.y + instanceSpecificRand));
    float loopSpecificRand1 = randRange(vec2(gl_InstanceIndex, loopInt));
    float loopSpecificRand2 = randRange(vec2(loopInt, gl_InstanceIndex));
    vec2 p0 = ubo.portalPoints.xy;
    vec2 p1 = ubo.portalPoints.zw;
    vec2 midPos = (p0 + p1) * 0.5;
    vec2 pointsDiff = p1 - p0;
    float portalSize = length(pointsDiff);
    vec2 normPointDiff = pointsDiff / portalSize;
    vec2 dirVec = rotate90(normPointDiff);

    portalSize -= ubo.data1.x;
    p0 += normPointDiff * ubo.data1.x * 0.5;
    p1 -= normPointDiff * ubo.data1.x * 0.5;
    pointsDiff = p1 - p0;

    vec2 fromPos;
    vec2 toPos;
    if (ubo.color.w > 0.0)
    {
        fromPos = p0 + pointsDiff * loopSpecificRand1;
        toPos = midPos + dirVec * (ubo.data0.x + (instanceSpecificRand - 0.5) * 0.2) + normPointDiff * (loopSpecificRand1 - 0.5) * ubo.data0.w;
    }
    else
    {
        fromPos = p1 - pointsDiff * loopSpecificRand1;
        toPos = midPos + dirVec * (ubo.data0.x + (instanceSpecificRand - 0.5) * 0.2) + normPointDiff * (loopSpecificRand1 - 0.5) * ubo.data0.w * -1.0;
    }

    vec2 currentPos = fromPos + (toPos - fromPos) * loopTime;

    worldPos = vertex * ubo.data1.xy + currentPos;
    gl_Position = sharedUboData.cameraMatrix * vec4(worldPos, 0.0, 1.0);
    
    localPos = vertex;
    colour = ubo.color + (instanceSpecificRand - 0.5) * 0.75;

    if (time < 0.0)
    {
        colour.w = 0.0;
    }
    else
    {
        colour.w = pow(1.0 - loopTime, 2.0);
    }
}