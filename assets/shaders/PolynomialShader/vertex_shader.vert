#version 460 core

#define PI 3.141592653589793238

layout (location = 0) in vec2 vertex;

layout (location = 0) out vec2 localPos;
layout (location = 1) out vec2 worldPos;
layout (location = 2) flat out vec4 equationData;
layout (location = 3) out float arcPos;
layout (location = 4) flat out vec4 outData;


layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 orthoMatrix;
    vec4 cameraPos;
    vec4 data; // zoom, unused, unused, unused
} sharedUboData;

layout (push_constant) uniform UBO
{
    // ax^2 + bx + c
    vec4 equation; // a, b, c, thickness
    vec4 data; // vertXStart, start, xLen, alphaMult
} ubo;

float getY(float x)
{
    return ubo.equation.x * pow(x, 2.0) + ubo.equation.y * x + ubo.equation.z;
}

vec2 getDer(float x)
{
    return normalize(vec2(1.0, ubo.equation.x * 2.0 * x + ubo.equation.y));
}

void main()
{
    localPos = vertex;
    equationData = ubo.equation;
    outData = ubo.data;

    float x, y;
    vec2 der;

    if (ubo.data.x < -7363748397.90326883)
    {
        x = ubo.data.y + vertex.x * ubo.data.z;
        y = getY(x);
        der = getDer(x);
        der = vec2(-der.y, der.x);
        arcPos = (x - ubo.data.y) * sign(ubo.data.z);
    }
    else
    {
        x = ubo.data.x;
        y = ubo.data.y + vertex.x * ubo.data.z;
        der = vec2(1.0, 0.0);
        arcPos = (y - ubo.data.y) * 0.05 * sign(ubo.data.z);
        outData.z *= 0.05;
    }

    worldPos = vec2(x, y) + der * ubo.equation.w * vertex.y;

    gl_Position = sharedUboData.orthoMatrix * vec4(worldPos, 0.0, 1.0);
}