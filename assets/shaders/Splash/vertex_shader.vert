#version 460 core

#define PI 3.14159265359
#define PI2 PI * 2.0

layout (location = 0) out vec2 localPosition;
layout (location = 1) flat out vec2 speed;
layout (location = 2) flat out vec2 outP;
layout (location = 3) flat out float outR;
layout (location = 4) flat out float curveMult;
layout (location = 5) flat out float dLen;
layout (location = 6) flat out float seeAng;
layout (location = 7) flat out float r;
layout (location = 8) flat out vec2 data; // dt, clr

layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 orthoMatrix;
    vec4 cameraPos;
    vec4 data; // zoom, unused, unused, unused
} sharedUboData;

layout (push_constant) uniform PC
{
	vec4 posSpeed;
    vec4 data; // radius, summonTime, angleMult, angleOffset
    vec4 data2; // pow, powMult, speedMult, outRMult
    vec4 data3; // addMult, fadeoutTime, dtMod, splashRadius
    vec4 color;
} pc;

ivec2 indexArray[6] = 
{
    ivec2(0, 0),
    ivec2(1, 0),
    ivec2(1, 1),
    ivec2(1, 1),
    ivec2(0, 1),
    ivec2(0, 0)
};


vec2 rotateVec(vec2 vec, float angle)
{
    float x = cos(angle) * vec.x - sin(angle) * vec.y;
    float y = sin(angle) * vec.x + cos(angle) * vec.y;

    return vec2(x, y);
}

float rand(vec2 seed)
{
    float random = fract(sin(dot(seed, vec2(13.8695, 17.1371))) * 73714.38361);
    
    return random;
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

void main() 
{
    const float gravity = 5.0;
    const float reqR = pc.data3.w;
    float rander = pc.data.y * 1.071 + gl_InstanceIndex * 1.453;
    float rnd = rand(vec2(rander * 1.881));
    float rnd2 = rand(vec2(rander * 2.023));
    r = pc.data.x * (0.31 + rnd2 * 0.69);
    vec2 randVec = rotateVec(vec2(1.0, 0.0), rnd * pc.data.z + pc.data.w);
    float randR = rnd2 * reqR * (1.0 - r / reqR) + r;

    vec2 minMax[2];

    float now = sharedUboData.data.y;
    float startTime = pc.data.y;
    float dt = mod(now - startTime, pc.data3.z * (0.5 + rnd * rnd2 * 2.0));
    data.x = dt;
    data.y = rand(vec2(rander * 2.331));

    if ((max(pc.data3.y - (data.y + 1.0) * data.x, 0.0) / pc.data3.y) <= 0.0) gl_Position = vec4(0.0);

    speed = randVec * pc.data2.z;
    speed.y *= rand(vec2(rander * 3.76534));
    vec2 pos = pc.posSpeed.xy + randVec * randR;
    pos.x += speed.x * dt;
    pos.y += (2.0 * speed.y - dt * gravity) * dt * 0.5;
    speed.y -= dt * gravity;
    speed *= 0.5;
    
    curveMult = speed.x < 0.0 ? -1.0 : 1.0;
    dLen = max(length(speed) * 0.5 * pc.data2.w, r);
    outR = min(max(dLen * max(abs(speed.x / speed.y), abs(speed.y / speed.x)), r * 5.0), 1000.0);
    outP = pos + normalize(vec2(speed.y, -speed.x)) * outR * curveMult;
    seeAng = min(dLen / outR, PI * 0.5);

    minMax[0].x = outP.x - outR - r;
    minMax[0].y = outP.y - outR - r;

    minMax[1].x = outP.x + outR + r;
    minMax[1].y = outP.y + outR + r;

    vec2 v0 = normalize(rotateVec(pos - outP, curveMult * dLen / (outR - r)));
    vec2 v1 = normalize(rotateVec(pos - outP, -curveMult * r / (outR - r)));

    vec2 p0 = outP + v0 * (outR + r);
    vec2 p1 = outP + v0 * (outR - r);

    vec2 p2 = outP + v1 * (outR + r);
    vec2 p3 = outP + v1 * (outR - r);

    minMax[0].x = min(min(min(p0.x, p1.x), p2.x), p3.x);
    minMax[0].y = min(min(min(p0.y, p1.y), p2.y), p3.y);

    minMax[1].x = max(max(max(p0.x, p1.x), p2.x), p3.x);
    minMax[1].y = max(max(max(p0.y, p1.y), p2.y), p3.y);

    ivec2 ind = indexArray[gl_VertexIndex];
    vec2 wp = vec2(minMax[ind[0]].x, minMax[ind[1]].y);
    localPosition = wp - pos;
    outP -= pos;

    gl_Position = sharedUboData.orthoMatrix * vec4(wp, 0.0, 1.0);
}