#version 460 core

#define PI 3.14159265359

layout (location = 0) out vec4 FragColor;

layout (location = 0) in vec2 localPos;
layout (location = 1) in vec2 worldPos;

layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 orthoMatrix;
    vec4 cameraPos;
    vec4 data; // zoom, unused, unused, unused
} sharedUboData;

layout (push_constant) uniform PC
{
    vec4 posSize;
    int data[4];
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
    float md = min(hDiff, vDiff);
    float minDiff = multFunc(max((v - md) / v, 0.0));

    const float v2 = 9.0;
    const float ldm = 0.002 * pc.posSize.z;
    float lineDiffX = mod(abs(localPos.x), v2);
    if (lineDiffX > v2 * 0.5) lineDiffX = v2 - lineDiffX;

    lineDiffX /= v2 * 0.5;
    lineDiffX = 1.0 - lineDiffX;
    lineDiffX = max((lineDiffX - (1.0 - ldm)) / ldm, 0.0);

    const float v3 = 9.0;
    float lineDiffY = mod(abs(localPos.y), v3);
    if (lineDiffY > v3 * 0.5) lineDiffY = v3 - lineDiffY;

    lineDiffY /= v3 * 0.5;
    lineDiffY = 1.0 - lineDiffY;
    lineDiffY = max((lineDiffY - (1.0 - ldm)) / ldm, 0.0);

    if (md < 0.5)
    {
        lineDiffX = 0.0;
        lineDiffY = 0.0;
    }

    float sqDiff = 0.0;

    if (pc.data[0] < 2 && pc.data[0] >= 0 && pc.data[1] < 8 && pc.data[1] >= 0)
    {
        float gridX = (9.0 * pc.data[0] - pc.posSize.z * 0.5 + 0.5) + 4.5f;
        float gridY = (-9.0 * pc.data[1] + pc.posSize.w * 0.5 - 0.5) - 4.5f;

        float sqXDiff = abs(gridX - localPos.x);
        float sqYDiff = abs(gridY - localPos.y);

        const float sv = 4.4;
        if (sqXDiff < sv && sqYDiff < sv)
        {
             sqDiff = 1.0;
        }
    }

	FragColor = vec4(borderColour * minDiff, 0.95) + (lineDiffX + lineDiffY) * 0.05 + vec4(vec3(sqDiff * 0.1), 0.0);
}