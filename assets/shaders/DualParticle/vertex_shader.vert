#version 460 core

#define PI 3.14159265359

layout (location = 0) in vec2 vertex;

layout (location = 0) out vec2 localPos;
layout (location = 1) out vec2 size;
layout (location = 2) out vec3 color0;
layout (location = 3) out vec3 color1;
layout (location = 4) out float circleRadius;

layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 cameraMatrix;
    vec4 cameraPos; // pos.xy, size.xy
    vec4 data;
} sharedUboData;

layout (std140, set = 1, binding = 0) uniform UBO
{
    vec4 color0;
    vec4 color1;
    vec4 data0; // none, speed, time, midCircleRadius
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
    float random = fract(sin(dot(seed, vec2(13.8695 + ubo.data1.z, 17.1371))) * 73714.38361);
    
    return random;
}

void main()
{
    float instanceSpecificRand = randRange(vec2(gl_InstanceIndex, gl_InstanceIndex));
    float scaleMult = (instanceSpecificRand + 2.0) * 0.3;
    localPos = vertex * ubo.data1.xy;
    vec2 rotatedLocalPos = rotatePoint(localPos, instanceSpecificRand * 2.0 * PI);

    float multTime = sharedUboData.data.y * (ubo.data0.y + instanceSpecificRand * 0.4) + instanceSpecificRand * 41241.0;
    float modedTime = fract(multTime);

    float loopInstanceRand0 = randRange(vec2(int(multTime), gl_InstanceIndex));
    float loopInstanceRand1 = randRange(vec2(gl_InstanceIndex, int(multTime)));

    vec2 currentSize;
    currentSize.x = ubo.data1.x * pow(1.0 - modedTime, 3.0) * scaleMult;
    float val2 = sin(modedTime * PI);
    currentSize.y = ubo.data1.y * val2 * scaleMult;
    size = currentSize;
    color0 = ubo.color0.xyz + (instanceSpecificRand - 0.5) * 0.75;
    color1 = ubo.color1.xyz + (instanceSpecificRand - 0.5) * 0.75;
    circleRadius = ubo.data0.w * scaleMult;

    mat4 newCameraMatrix = sharedUboData.cameraMatrix;
    newCameraMatrix[3][0] *= scaleMult;
    newCameraMatrix[3][1] *= scaleMult;

    vec2 camPos = vec2(sharedUboData.cameraPos.x, -sharedUboData.cameraPos.y);
    vec2 camSizes = sharedUboData.cameraPos.zw;
    vec2 particlePos = vec2(loopInstanceRand0 * 200.0 - 100.0, loopInstanceRand1 * 200.0 - 100.0);
    particlePos -= camPos * scaleMult;

    const float xCopySize = 50.0;
	const float yCopySize = 30.0;

    particlePos.x = mod(particlePos.x + xCopySize, xCopySize * 2.0) - xCopySize;
    particlePos.y = mod(particlePos.y + yCopySize, yCopySize * 2.0) - yCopySize;

	particlePos += camPos * scaleMult;

    vec4 outPos = newCameraMatrix * vec4(rotatedLocalPos + particlePos, 0.0, 1.0);

    gl_Position = outPos;
}