#version 460 core

layout (location = 0) in vec2 vertex;

layout (location = 0) out vec2 localPos;
layout (location = 1) out vec2 worldPosition;

layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 orthoMatrix;
    vec4 cameraPos;
    vec4 data; // zoom, unused, unused, unused
} sharedUboData;

// use per PortalBody
layout (std140, set = 1, binding = 0) uniform SeperateUBO
{
    vec4 posRotD; // posx, posy, rotation, unused
    vec4 portals[4];
    int numPortals;
} s_ubo;

struct legData
{
    float legAngle;
    float legThick;
    float circR;
    float legLen;
    float legOuterCircAng;
    float legCircleSize;
};

// common for everybody
layout (std140, set = 1, binding = 1) uniform CommonUbo
{
    vec4 edgeColour;
    vec4 bodyColour1;
    vec4 bodyColour2;
    vec2 size;
    vec2 headCircles;
    float topMargin;
    float headRatio;
    float bodyRatio;
    float legRatio;
    float margin1;
    float margin2;
    float lineSize;
    float bodyCircles;
    legData ld;
} c_ubo;

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
	localPos = vertex * vec2(0.5, 1.0);
    worldPosition = rotateAroundOrigin(vertex * c_ubo.size, s_ubo.posRotD.z) + s_ubo.posRotD.xy;

    gl_Position = sharedUboData.orthoMatrix * vec4(worldPosition, 0.0, 1.0);
}