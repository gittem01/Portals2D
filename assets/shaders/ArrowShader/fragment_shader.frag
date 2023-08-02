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
    vec4 color;
    vec4 data; // h1x, h2x, vx, vy
    vec4 data2; // rot, cr1, cr2, cr3
} pc;

float normalizeAngle(float angle)
{
    angle = mod(angle, 2.0 * PI);
    if (angle < 0.0) angle += 2.0 * PI;
    return angle;
}

bool angleBetween(float a, float b, float angle)
{
    a = normalizeAngle(a);
    b = normalizeAngle(b);
    angle = normalizeAngle(angle);
    
    if (a > b)
    {
        b += 2.0 * PI;
        if (angle < a) angle += 2.0 * PI;
    }
    
    if (angle > a && angle < b) return true;
    else return false;
}

float calcAngle2(vec2 vec)
{
    float angle = atan(vec.y, vec.x);
    if (angle < 0.0) angle += PI * 2.0;

    return angle;
}

float vecAngle(vec2 v1, vec2 v2)
{
    return abs(normalizeAngle(calcAngle2(v1) - calcAngle2(v2)));
}

vec2 rotateVec(vec2 vec, float angle)
{
    float x = cos(angle) * vec.x - sin(angle) * vec.y;
    float y = sin(angle) * vec.x + cos(angle) * vec.y;

    return vec2(x, y);
}

void circBetween(   in vec2 p1, in vec2 p2, in vec2 p3, in float dist, out vec2 p, out float r,
                    out float ang1, out float ang2)
{
    
    float angBtw = vecAngle(p1 - p2, p3 - p2);
    float halfAngle = angBtw * 0.5;
    vec2 midVec = normalize(rotateVec(p3 - p2, halfAngle)) * dist;
    
    p = p2 + midVec;
    r = sin(halfAngle) * dist;
    
    float revHalfAngle = PI * 0.5 - halfAngle;
    float vAng = calcAngle2(p1 - p2);
    ang2 = vAng + PI * 0.5;
    ang1 = ang2 + 2.0 * revHalfAngle;
}

float getDist(vec2 a, vec2 b, vec2 c)
{
    float v1 = (b.x - a.x) * (a.y - c.y);
    float v2 = (a.x - c.x) * (b.y - a.y);
    float top = v1 - v2;

    v1 = pow(b.x - a.x, 2.0);
    v2 = pow(b.y - a.y, 2.0);
    float bot = sqrt(v1 + v2);

    return top / bot;
}

float retMult(float dist)
{
    const bool emptyInside = false;
    const float v = pc.posSize.z * 0.03 / sharedUboData.data.x;
    
    if (dist > v && emptyInside)
    {
        return 1.0 - pow((v - dist) / v, 2.0);
    }
    
    return min(pow(dist / v, 2.0), 1.0);
}

void main()
{
    vec4 clr = pc.color;
    
    vec2 hPos1 = vec2(pc.data.x, 0.0);
    vec2 hPos2 = vec2(pc.data.y, 0.0);
    vec2 vPos = pc.data.zw;
    
    if (vPos.x > hPos2.x) vPos.x = hPos2.x - 0.0001;
    
    vec2 lPos = vec2(localPos.x, abs(localPos.y));

    float dist0 = -1.0;
    
    vec2 p;
    float r, ang1, ang2;
    if (hPos1.x > vPos.x)
    {
        circBetween(vec2(vPos.x, -vPos.y), hPos1, vPos, pc.data2.w, p, r, ang1, ang2);
    
        if (angleBetween(ang2, ang1, calcAngle2(lPos - p)))
        {
            float l = length(lPos - p);
            if (l > r)
            {
                dist0 = l - r;
            }
        }
    }
    
    float dist2 = getDist(vPos, hPos2, lPos);
    float dist3 = getDist(hPos1, vPos, lPos);
    
    if (dist2 > 0.0 && dist3 > 0.0 || (dist0 > 0.0 && dist3 < 0.0))
    {
        if (dist0 < 0.0) dist0 = 3153617847165241.0;
        else dist3 = 3153617847165241.0;
        
        float minDist = min(min(dist0, dist2), dist3);
        FragColor = retMult(minDist) * clr;
        
        circBetween(vec2(vPos.x, -vPos.y), hPos2, vPos, pc.data2.y, p, r, ang1, ang2);
        
        if (angleBetween(ang2, ang1, calcAngle2(lPos - p)))
        {
            if (length(lPos - p) < r)
            {
                minDist = min(r - length(lPos - p), minDist);
                FragColor = retMult(minDist) * clr;
            }
            else
            {
                FragColor = vec4(0.0);
            }
        }
        
        circBetween(hPos2, vPos, hPos1, pc.data2.z, p, r, ang1, ang2);
        if (angleBetween(ang2, ang1, calcAngle2(lPos - p)))
        {
            if (length(lPos - p) < r)
            {
                minDist = min(r - length(lPos - p), minDist);
                FragColor = retMult(minDist) * clr;
            }
            else
            {
                FragColor = vec4(0.0);
            }
        }
        
        if (hPos1.x < vPos.x)
        {
            circBetween(vPos, hPos1, vec2(vPos.x, -vPos.y), pc.data2.w, p, r, ang1, ang2);

            if (angleBetween(ang2, ang1, calcAngle2(lPos - p)))
            {
                if (length(lPos - p) < r)
                {
                    minDist = min(r - length(lPos - p), minDist);
                    FragColor = retMult(minDist) * clr;
                }
                else
                {
                    FragColor = vec4(0.0);
                }
            }
        }
        
        return;
    }
    
    FragColor = vec4(0.0);
}