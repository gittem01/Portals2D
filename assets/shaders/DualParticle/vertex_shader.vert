#version 460 core

#define PI 3.14159265359

layout (location = 0) out vec2 localPos;
layout (location = 1) out vec2 size;
layout (location = 2) out vec3 color0;
layout (location = 3) out vec3 color1;
layout (location = 4) out float circleRadius;

layout (set = 0, binding = 0) uniform SharedpcData
{
    mat4 cameraMatrix;
    vec4 cameraPos; // pos.xy, size.xy
    vec4 data;
} sharedpcData;

layout (push_constant) uniform PC
{
    vec4 color0;
    vec4 color1;
    vec4 data0; // none, speed, time, midCircleRadius
    vec4 data1; // sizex, sizey, randomization, none
} pc;

vec2 rotatePoint(vec2 point, float angle)
{
    float x = cos(angle) * point.x - sin(angle) * point.y;
    float y = sin(angle) * point.x + cos(angle) * point.y;

    return vec2(x, y);
}

vec2 rotate90(vec2 v)
{
    return vec2(-v.y, v.x);
}

float randRange(vec2 seed)
{
    float random = fract(sin(dot(seed, vec2(1.8695 + pc.data1.z, 1.1371))) * 7.38361);
    
    return random;
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
    vec2 vertex = vertices[gl_VertexIndex];

    float instanceSpecificRand = randRange(vec2(gl_InstanceIndex, gl_InstanceIndex));
    float scaleMult = (instanceSpecificRand + 2.0) * 0.3;
    localPos = vertex * pc.data1.xy;
    vec2 rotatedLocalPos = rotatePoint(localPos, instanceSpecificRand * 2.0 * PI);

    float multTime = sharedpcData.data.y * (pc.data0.y + instanceSpecificRand * 0.4) + instanceSpecificRand * 1.54243;
    float modedTime = fract(multTime);

    float loopInstanceRand0 = randRange(vec2(int(multTime), gl_InstanceIndex));
    float loopInstanceRand1 = randRange(vec2(gl_InstanceIndex, int(multTime)));

    vec2 currentSize;
    currentSize.x = pc.data1.x * pow(1.0 - modedTime, 3.0) * scaleMult;
    float val2 = sin(modedTime * PI);
    currentSize.y = pc.data1.y * val2 * scaleMult;
    size = currentSize;
    color0 = pc.color0.xyz + (instanceSpecificRand - 0.5) * 0.75;
    color1 = pc.color1.xyz + (instanceSpecificRand - 0.5) * 0.75;
    circleRadius = pc.data0.w * scaleMult;

    mat4 newCameraMatrix = sharedpcData.cameraMatrix;
    newCameraMatrix[3][0] *= scaleMult;
    newCameraMatrix[3][1] *= scaleMult;

    vec2 camPos = vec2(sharedpcData.cameraPos.x, -sharedpcData.cameraPos.y);
    vec2 camSizes = sharedpcData.cameraPos.zw;
    vec2 particlePos = vec2(loopInstanceRand0 * 200.0 - 100.0, loopInstanceRand1 * 200.0 - 100.0);
    particlePos -= camPos * scaleMult;

    const float xCopySize = camSizes.x + 2.0;
	const float yCopySize = camSizes.y + 2.0;

    particlePos.x = mod(particlePos.x + xCopySize, xCopySize) - xCopySize * 0.5;
    particlePos.y = mod(particlePos.y + yCopySize, yCopySize) - yCopySize * 0.5;

	particlePos += camPos * scaleMult;

    vec4 outPos = newCameraMatrix * vec4(rotatedLocalPos + particlePos, 0.0, 1.0);

    gl_Position = outPos;
}