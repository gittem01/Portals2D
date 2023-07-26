#version 460 core

#define PI 3.14159265359

layout (location = 0) out vec4 FragColor;

layout (location = 0) in vec2 localPos;

layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 orthoMatrix;
    vec4 cameraPos;
    vec4 data; // zoom, unused, unused, unused
} sharedUboData;

layout (push_constant) uniform PC
{
    vec4 posSize;
    vec4 data;
} pc;

float multFunc(float diff)
{
    return sin(diff * PI);
}

void main()
{
    vec3 borderColour = vec3(0.1, 0.1, 0.1);
    float hDiff = pc.posSize.z * 0.5 - abs(localPos.x);
    float vDiff = pc.posSize.w * 0.5 - abs(localPos.y);

    const float v = 0.5;
    float minDiff = multFunc(max((v - min(hDiff, vDiff)) / v, 0.0));

	FragColor = vec4(borderColour * minDiff, 0.95);
}