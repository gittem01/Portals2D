#version 460 core

#define PI 3.14159265359

layout (location = 0) in vec2 vertex;

layout (location = 0) out vec2 localPos;

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
    vec4 data0; // dist, speed, time, angle
    vec4 data1; // sizex, sizey, randomization, none
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
    float random = fract(sin(dot(seed, vec2(63.8695, 59.1371))) * 73714.38361);
    
    return random;
}

void main()
{
    float instanceSpecificRand = randRange(vec2(gl_InstanceIndex, gl_InstanceIndex));
    float time = ubo.data0.z;
    float loopTime = fract(time * (ubo.data0.y + instanceSpecificRand));
    int loopInt = int(time * (ubo.data0.y + instanceSpecificRand));
    float loopSpecificRand1 = randRange(vec2(gl_InstanceIndex, loopInt));
    float loopSpecificRand2 = randRange(vec2(loopInt, gl_InstanceIndex));
    vec2 p0 = ubo.portalPoints.xy;
    vec2 p1 = ubo.portalPoints.zw;
    vec2 pointsDiff = p1 - p0;
    float portalSize = length(pointsDiff);
    vec2 normPointDiff = pointsDiff / portalSize;
    vec2 dirVec = rotate90(normPointDiff);

    vec2 fromPos = p0 + pointsDiff * loopSpecificRand1;
    vec2 toPos = p0 + dirVec * ubo.data0.x + pointsDiff * loopSpecificRand2;
    vec2 currentPos = fromPos + (toPos - fromPos) * loopTime;

    float randX = randRange(vec2(gl_InstanceIndex * 2.3251, gl_InstanceIndex * 1.41341)) * 4.0;
    float randY = randRange(vec2(gl_InstanceIndex * 4.14124,  gl_InstanceIndex * 9.314241)) * 4.0;

    gl_Position = sharedUboData.cameraMatrix * vec4(vertex * ubo.data1.xy + currentPos, 0.0, 1.0);
    
    localPos = vertex;
}