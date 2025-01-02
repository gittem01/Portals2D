#version 460 core

#define PI 3.14159265359

layout (location = 0) out vec4 outFragColor;

layout (location = 0) in vec2 localPosition;
layout (location = 1) flat in vec2 speed;
layout (location = 2) flat in vec2 outP;
layout (location = 3) flat in float outR;
layout (location = 4) flat in float curveMult;
layout (location = 5) flat in float seeAng;
layout (location = 6) flat in float r;
layout (location = 7) flat in vec2 data; // dt, clr

layout(set = 1, binding = 0) uniform sampler2D tex;

layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 cameraMatrix;
    vec4 cameraPos;
    vec4 data;
} sharedUboData;

layout (push_constant) uniform PC
{
	vec4 posSpeed;
    vec4 data; // radius, summonTime, angleMult, angleOffset
    vec4 data2; // pow, powMult, speedMult, outRMult
    vec4 data3; // addMult, fadeoutTime, dtMod, splashRadius
    vec4 color;
} pc;

float sdCircle( vec2 p, float r )
{
    return length(p) - r;
}

vec2 rotateVec(vec2 vec, float angle)
{
    float x = cos(angle) * vec.x - sin(angle) * vec.y;
    float y = sin(angle) * vec.x + cos(angle) * vec.y;

    return vec2(x, y);
}

float normalizeAngle(float angle)
{
    angle = mod(angle, 2.0 * PI);
    if (angle < 0.0) angle += 2.0 * PI;
    return angle;
}

bool angleBetween(float a, float b, float angle, bool rev, out float angDiff)
{
    a = normalizeAngle(a);
    b = normalizeAngle(b);
    angle = normalizeAngle(angle);
    
    if (a > b)
    {
        b += 2.0 * PI;
        if (angle < a) angle += 2.0 * PI;
    }
    
    if (!rev) angDiff = angle - a;
    else angDiff = normalizeAngle(a - angle);
    if (angle > a && angle < b) return !rev;
    else return rev;
}

bool isBetweenVectors(vec2 a, vec2 b, vec2 p, bool rev, out float angDiff)
{
    float angleA = normalizeAngle(atan(a.y, a.x));
    float angleB = normalizeAngle(atan(b.y, b.x));
    float angleP = normalizeAngle(atan(p.y, p.x));
    
    return angleBetween(angleA, angleB, angleP, rev, angDiff);
}

void main()
{
    vec2 uv = localPosition;
    
    vec2 pos = vec2(0.0);
    vec2 diff = pos - outP;
    
    float v = (r - abs(sdCircle(uv - outP, outR)));

    float angDiff;    
    bool check = isBetweenVectors(diff, rotateVec(diff, seeAng * curveMult), uv - outP, curveMult < 0.0, angDiff);
    
    float v2 = -sdCircle(uv - pos, r);

    float dropAmnt = pow(angDiff / seeAng, 2.0) * float(check);
    v = (max(v2, float(check) * v) - dropAmnt * r) / r;
    v = max(abs(pow(v, pc.data2.x)) * sign(v) * pc.data2.y, 0.0);
    vec4 clr = vec4(vec3(normalize(pc.color.xyz * v) + pc.data3.x * v), pc.color.w * min(v * (max(pc.data3.y - (data.y + 1.0) * data.x, 0.0) / pc.data3.y), 1.0));
    
    outFragColor = clr;
}