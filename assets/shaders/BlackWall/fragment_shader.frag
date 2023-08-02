#version 460 core

#define PI 3.14159265359
#define SMOOTHER 1.0

#define VALFUNC(x) -cos(x) * 0.5 * SMOOTHER
#define INTRL(x) 0.5 * (sin(x) + x) * SMOOTHER

layout (location = 0) out vec4 FragColour;

layout (location = 0) in vec2 localPos;
layout (location = 1) in vec2 worldPos;
layout (location = 2) flat in int instanceIndex;

layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 cameraMatrix;
    vec4 cameraPos;
    vec4 data;
} sharedUboData;

struct storageData
{
    vec4 posSize;
    vec4 colour;
    vec4 data; // isVertical, lineSize, camMult, none
};

layout (std140, set = 1, binding = 0) readonly buffer UBO
{
    storageData sData[];
} ubo;

float valBetween(float a, float b)
{
    return VALFUNC(b) - VALFUNC(a);
}

float integralBtw(float a, float b)
{
    return INTRL(b) - INTRL(a);
}

vec2 cosDer(float x, float range)
{
    float s = valBetween(x - range, x + range) / (range * 2.0);
    return normalize(vec2(-s, 1.0));
}

void main()
{
    storageData data = ubo.sData[instanceIndex];

    vec3 camPos = vec3(sharedUboData.cameraPos.x * data.data.z, -sharedUboData.cameraPos.y * data.data.z, sharedUboData.cameraPos.z / data.data.z);
    vec4 color = data.colour;
    float lineSize = data.data.y;
    vec4 borderColor = vec4(0.0, 0.0, 0.0, 1.0);
    float borderSize = 0.05;
    bool isVertical = data.data.x > 0.5;;

    float m = 1.0;
    if (worldPos.x > 5.0 && worldPos.x < 15.0 &&
        worldPos.y > 3.0 && worldPos.y < 7.0 || true)
    {
        const float pVal = 4.0;
        m = 0.0;
        m += pow(max(1.0 - (worldPos.x + 4.0), 0.0), pVal);
        m += pow(max(1.0 - (15.0 - worldPos.x), 0.0), pVal);
        m += pow(max(1.0 - (worldPos.y - 3.0), 0.0), pVal);
        m += pow(max(1.0 - (7.0 - worldPos.y), 0.0), pVal);
        m = min(m, 1.0);

        float adder = 0.02;
        m += adder;
        m *= 1.0 / (1.0 + adder);
    }

    vec3 fragPos = vec3(worldPos, 0.0);

    float ratio = 0.0;
    if (abs(localPos.x) > data.posSize.z * 0.5 - borderSize)
    {
        float diff = abs(localPos.x) - (data.posSize.z * 0.5 - borderSize);
        ratio = diff / borderSize;
    }
    if (abs(localPos.y) > data.posSize.w * 0.5 - borderSize)
    {
        float diff = abs(localPos.y) - (data.posSize.w * 0.5 - borderSize);
        ratio = max(diff / borderSize, ratio);
    }

    ratio += 0.25;
    if (ratio > 1.0) ratio = 1.0;

    float val01;
    if (isVertical)
    {
        val01 = (localPos.x * 2.0 * PI) / lineSize;
    }
    else
    {
        val01 = (localPos.y * 2.0 * PI) / lineSize;
    }
    
    float range = camPos.z * 0.003 / lineSize;
    float mult = integralBtw(val01 - range, val01 + range) / (range * 2.0);

    vec2 normVec = cosDer(val01, range * 5.0);
    vec3 surfaceNormal;
    if (isVertical)
        surfaceNormal = vec3(normVec.x, 0.0, normVec.y);
    else
        surfaceNormal = vec3(0.0, normVec.x, normVec.y);
    vec3 camFragVec = fragPos - camPos;
    vec3 contVec = reflect(normalize(vec3(0.0, 0.0, 1.0)), surfaceNormal);
    
    float brightness = dot(normalize(contVec), normalize(camFragVec));
    brightness = pow(brightness, 40.0) * 1.0;
    
    FragColour = (color * (1.0 - mult) + color * brightness) * (1.0 - ratio) * m + borderColor * ratio * m;
}