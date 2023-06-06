#version 460 core

#define PI 3.14159265359
#define POW_VAL 3.0
#define ETA 0.66

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

struct uboData
{
    vec4 posSize;
    vec4 color;
    vec4 data; // lightXSize, lightYSize, upDownSize, lightDepth
    vec4 data2; // rotation, glassRotation (added on top of rotation), none, none
};

layout (set = 1, binding = 0) readonly buffer UBO
{
    uboData data[];
} ubo;

float getDist(vec2 b, vec2 c)
{
    float v1 = b.x * -c.y;
    float v2 = -c.x * b.y;
    float top = v1 - v2;

    v1 = pow(b.x, 2);
    v2 = pow(b.y, 2);
    float bot = sqrt(v1 + v2);

    return top / bot;
}

vec2 cosDer(float x)
{
    float smoother = 0.3;
    float s = pow(0.5, POW_VAL) * POW_VAL * sin(x) * pow(cos(x) + 1.0, POW_VAL - 1.0) * smoother;
    return normalize(vec2(s, 1.0));
}

vec2 rotatePoint(vec2 point, float angle)
{
    float x = cos(angle) * point.x - sin(angle) * point.y;
    float y = sin(angle) * point.x + cos(angle) * point.y;

    return vec2(x, y);
}

void main()
{
    uboData data = ubo.data[instanceIndex];
    vec4 lightColor = data.color;
    float upDownSize = data.data.z;
    float xSize = data.data.x;
    float ySize = data.data.y;
    float lightDepth = data.data.w;
    vec3 camPos = vec3(sharedUboData.cameraPos.x, -sharedUboData.cameraPos.y, sharedUboData.cameraPos.z);
    vec2 reachFrag = localPos;
    
    vec2 distVec = rotatePoint(vec2(0.0, 1.0), data.data2.y);
    float v0 = mod(localPos.x, upDownSize) / upDownSize;
    v0 = mod(getDist(distVec, localPos), upDownSize) / upDownSize;
    float sinVal = 1.0 - pow((cos(v0 * PI * 2.0) + 1.0) * 0.5, POW_VAL);
       
    vec2 normVec = cosDer(v0 * PI * 2.0);
    vec2 v2 = rotatePoint(vec2(normVec.x, 0.0), data.data2.x + data.data2.y);
    vec3 surfaceNormal = vec3(v2, normVec.y);

    vec3 camFragVec = vec3(worldPos, 0.0) - camPos;
    vec3 contVec = refract(normalize(camFragVec), surfaceNormal, ETA);
    contVec.xy = rotatePoint(contVec.xy, -data.data2.x);
    float zTime = lightDepth / contVec.z;
    reachFrag += vec2(contVec.x * zTime, contVec.y * zTime);
    
    if (length(abs(reachFrag) - vec2(xSize * 0.5 - ySize * 0.5, 0.0)) < ySize * 0.5 ||
        (abs(reachFrag.y) < ySize * 0.5) && abs(reachFrag.x) < xSize * 0.5 - ySize * 0.5)
    {
        FragColour = lightColor;
    }
    else
    {
        float dist = length(abs(reachFrag) - vec2(xSize * 0.5 - ySize * 0.5, 0.0));
        if (abs(reachFrag.x) < xSize * 0.5 - ySize * 0.5)
        {
            dist = min(dist, abs(reachFrag.y));
        }
        float v =  1.0;
        dist -= ySize * 0.5;
        dist = max((v - dist) / v, 0.0);
        dist = pow(dist, 10.0);
        FragColour = vec4(dist) * lightColor + lightColor * vec4(sinVal) * 0.00;
    }
}