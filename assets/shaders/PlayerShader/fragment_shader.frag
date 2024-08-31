#version 460 core

#define PI 3.14159265359

layout (location = 0) out vec4 FragColour;

layout (location = 0) in vec2 localPos;
layout (location = 1) in vec2 worldPosition;

layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 orthoMatrix;
    vec4 cameraPos;
    vec4 data; // zoom, unused, unused, unused
} sharedUboData;

struct legData
{
    float legAngle;
    float legThick;
    float circR;
    float legLen;
    float legOuterCircAng;
    float legCircleSize;
};

// use per PortalBody
layout (std140, set = 1, binding = 0) uniform SeperateUBO
{
    vec4 posRotD; // posx, posy, rotation, unused
    vec4 portals[4];
    int numPortals;
} s_ubo;

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

vec4 getColour(float dist, float lineSize, vec4 innerColour, vec4 middleColour)
{
    dist = max(dist, 0.0);
    dist -= lineSize * 0.5;
    dist *= 2.0;
    float mult;
    if (dist > lineSize) mult = 1.0;
    else
    {
        mult = dist / lineSize;
        mult = pow((cos(mult * PI + PI) + 1.0) * 0.5, sharedUboData.data.x);
        if (dist < 0.0) mult *= -1.0;
    }
    
    if (mult > 0.0)
    {
        return innerColour * mult + middleColour * (1.0 - mult);
    }
    else
    {
        return middleColour * (1.0 + mult);
    }
}

float getDist(vec2 a, vec2 b, vec2 c)
{
    float v1 = (b.x - a.x) * (a.y - c.y);
    float v2 = (a.x - c.x) * (b.y - a.y);
    float top = abs(v1 - v2);

    v1 = pow(b.x - a.x, 2.0);
    v2 = pow(b.y - a.y, 2.0);
    float bot = sqrt(v1 + v2);

    return top / bot;
}

vec2 rotateAroundOrigin(vec2 point, float angle)
{
    float s = sin(angle);
    float c = cos(angle);

    vec2 p;

    p.x = point.x * c - point.y * s;
    p.y = point.y * c + point.x * s;

    return p;
}

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

float lineRel(vec2 a, vec2 b, vec2 c)
{
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

void main()
{
    int numPortals = s_ubo.numPortals;
    for (int i = 0; i < numPortals; i++)
    {
        if (lineRel(s_ubo.portals[i].xy, s_ubo.portals[i].zw, worldPosition) > 0)
        {
            FragColour = vec4(0.0f, 0.0f, 0.0f, 0.0f);
            return;
        }
    }

    float iTime = sharedUboData.data.y;
    vec2 size = vec2(0.5, 1.0);
    vec2 uv = localPos + size * 0.5;
    vec2 midUv = localPos;
    
    vec4 edgeColour = c_ubo.edgeColour;
    float uvVal = min(uv.y * 1.35, 1.0);
    vec4 bodyColour = c_ubo.bodyColour1 * uvVal + c_ubo.bodyColour2 * (1.0 - uvVal);

    float topMargin = c_ubo.topMargin;
    float headRatio = c_ubo.headRatio;
    float bodyRatio = c_ubo.bodyRatio;
    float legRatio = c_ubo.legRatio;
    float margin1 = c_ubo.margin1;
    float margin2 = c_ubo.margin2;
    float lineSize = c_ubo.lineSize / sharedUboData.data.x;
    vec2 headCircles = c_ubo.headCircles;
    float bodyCircles = c_ubo.bodyCircles;
    float legCircleSize = c_ubo.ld.legCircleSize;

    float legThick = c_ubo.ld.legThick;
    float circR = c_ubo.ld.circR;
    float legLen = c_ubo.ld.legLen;
    float legSizeAng = legLen / circR;

    float legOuterCircAngs[2];
    legOuterCircAngs[0] = c_ubo.ld.legOuterCircAng;
    legOuterCircAngs[1] = c_ubo.ld.legOuterCircAng + PI;

    vec2 legOuterCircles[2];
    legOuterCircles[0] = vec2(-cos(legOuterCircAngs[0]), -sin(legOuterCircAngs[0])) * circR;
    legOuterCircles[0] -= normalize(legOuterCircles[0]) * legRatio * 0.5;
    legOuterCircles[1] = vec2(-cos(legOuterCircAngs[1]), -sin(legOuterCircAngs[1])) * circR;
    legOuterCircles[1] -= normalize(legOuterCircles[1]) * legRatio * 0.5;

    float legAngles[2];
    legAngles[0] = c_ubo.ld.legAngle;
    legAngles[1] = c_ubo.ld.legAngle + PI;
    
    vec2 sqMults = vec2(-1.0, +1.0);
    
    float margins[6];
    float cmMargin[6];
    margins[0] = topMargin;
    margins[1] = headRatio;
    margins[2] = margin1;
    margins[3] = bodyRatio;
    margins[4] = margin2;
    margins[5] = legRatio;
    
    float s = 1.0;
    for (int i = 0; i < 6; i++)
    {
        s -= margins[i];
        cmMargin[i] = s;
    }

    if (uv.y < cmMargin[0] && uv.y > cmMargin[1])
    {
        float normY = uv.y - cmMargin[1] - size.x * 0.5 * 0.5;
        vec2 circPos = vec2(0.0, -size.x * 0.5 * 0.5);
        vec2 fPos = vec2(midUv.x, normY);
        
        float dist0 = size.x * 0.5 - length(fPos - circPos);
        dist0 = min(normY + size.x * 0.5 * 0.5, dist0);
        if (dist0 > 0.0)
        {
            for (int i = 0; i < 2; i++)
            {
                if (midUv.x > 0.0 && i == 0 || midUv.x < 0.0 && i == 1) continue;
                
                float xCent = sqrt(pow(size.x * 0.5 - headCircles[i], 2.0) - pow(headCircles[i], 2.0));
                float ang = atan(headCircles[i], xCent);
                vec2 c2pos = vec2(xCent, headCircles[i] - size.x * 0.5 * 0.5);
                vec2 diff = vec2(fPos.x * sqMults[i], fPos.y) - c2pos;
                float fAngle = atan(diff.y, diff.x);

                if (fAngle < ang && fAngle > -PI * 0.5)
                {
                    float dist1 = headCircles[i] - length(vec2(abs(fPos.x), fPos.y) - c2pos);
                    if (dist1 > 0.0)
                    {
                        FragColour = getColour(dist1, lineSize, bodyColour, edgeColour);
                    }
                    else
                    {
                        FragColour = vec4(0.0, 0.0, 0.0, 0.0);
                    }
                }
                else
                {
                    FragColour = getColour(dist0, lineSize, bodyColour, edgeColour);
                }
            }
        }
        else
        {
            FragColour = vec4(0.0, 0.0, 0.0, 0.0);
        }
        
        return;
    }
    else if (uv.y < cmMargin[2] && uv.y > cmMargin[3])
    {
        float normY = uv.y - cmMargin[3] - bodyRatio * 0.5;
        vec2 fPos = vec2(abs(midUv.x), abs(normY));
        vec2 cPos = vec2(size.x * 0.5 - bodyCircles, bodyRatio * 0.5 - bodyCircles);
        
        float dist;
        if (fPos.x > cPos.x && fPos.y > cPos.y)
        {
            dist = bodyCircles - length(cPos - fPos);
            if (dist < 0.0)
            {
                FragColour = vec4(0.0, 0.0, 0.0, 0.0);
                return;
            }
        }
        else
        {
            dist = min(size.x * 0.5 - fPos.x, bodyRatio * 0.5 - fPos.y);
        }
        
        FragColour = getColour(dist, lineSize, bodyColour, edgeColour);
        
        return;
    }
    else if (uv.y < cmMargin[4] && uv.y > cmMargin[5])
    {
        bool found = false;

        vec2 legOuterCircle;
        float legAngle;

        float normY = uv.y - cmMargin[5] - legRatio * 0.5;
        vec2 fPos = vec2(midUv.x, normY);

        vec2 diff = fPos - legOuterCircle;
        float ang = atan(diff.y, diff.x);
        for (int i = 0; i < 2; i++)
        {
            legOuterCircle = legOuterCircles[i];
            legAngle = legAngles[i];

            normY = uv.y - cmMargin[5] - legRatio * 0.5;
            fPos = vec2(midUv.x, normY);

            diff = fPos - legOuterCircle;
            ang = atan(diff.y, diff.x);

            if (angleBetween(legAngle - legSizeAng, legAngle + legSizeAng, ang))
            {
                float l = length(diff);
                float d1 = l - circR + legThick;
                float d2 = circR - l;
                if (d1 > 0.0 && d2 > 0.0)
                {
                    vec2 lVec1 = normalize(rotateAroundOrigin(vec2(1.0, 0.0), legAngle + legSizeAng));
                    float relDist = getDist(legOuterCircle, legOuterCircle + lVec1, fPos);

                    vec2 lVec2 = normalize(rotateAroundOrigin(vec2(1.0, 0.0), legAngle - legSizeAng));
                    relDist = min(getDist(legOuterCircle, legOuterCircle + lVec2, fPos), relDist);

                    float dist = min(min(d1, d2), relDist);

                    // circ1
                    float cd1 = sqrt(pow(circR - legCircleSize, 2.0) - pow(legCircleSize, 2.0));
                    vec2 p1 = legOuterCircle + lVec1 * cd1;
                    vec2 rotLvec1 = rotateAroundOrigin(lVec1, -PI * 0.5);
                    vec2 circOrigin = p1 + rotLvec1 * legCircleSize;
                    vec2 cv1 = normalize(circOrigin - legOuterCircle);
                    vec2 cv2 = normalize(p1 - circOrigin);
                    vec2 cvDiff1 = fPos - circOrigin;

                    float c1Ang1 = atan(cv1.y, cv1.x);
                    float c1Ang2 = atan(cv2.y, cv2.x);
                    float fAng = atan(cvDiff1.y, cvDiff1.x);

                    if (angleBetween(c1Ang1, c1Ang2, fAng))
                    {
                        dist = legCircleSize - length(cvDiff1);
                    }
                    else
                    {
                        // circ2
                        p1 = legOuterCircle + lVec2 * cd1;
                        vec2 rotLvec2 = rotateAroundOrigin(lVec2, -PI * 0.5);
                        circOrigin = p1 - rotLvec2 * legCircleSize;
                        cv1 = normalize(p1 - circOrigin);
                        cv2 = normalize(circOrigin - legOuterCircle);
                        cvDiff1 = fPos - circOrigin;

                        c1Ang1 = atan(cv1.y, cv1.x);
                        c1Ang2 = atan(cv2.y, cv2.x);
                        fAng = atan(cvDiff1.y, cvDiff1.x);

                        if (angleBetween(c1Ang1, c1Ang2, fAng))
                        {
                            dist = legCircleSize - length(cvDiff1);
                        }
                        else
                        {
                            // circ3
                            cd1 = sqrt(pow(circR + legCircleSize - legThick, 2.0) - pow(legCircleSize, 2.0));
                            p1 = legOuterCircle + lVec1 * cd1;
                            circOrigin = p1 + rotLvec1 * legCircleSize;
                            cv1 = normalize(p1 - circOrigin);
                            cv2 = normalize(legOuterCircle - circOrigin);
                            cvDiff1 = fPos - circOrigin;

                            c1Ang1 = atan(cv1.y, cv1.x);
                            c1Ang2 = atan(cv2.y, cv2.x);
                            fAng = atan(cvDiff1.y, cvDiff1.x);

                            if (angleBetween(c1Ang1, c1Ang2, fAng))
                            {
                                dist = legCircleSize - length(cvDiff1);
                            }
                            else
                            {
                                // circ4
                                p1 = legOuterCircle + lVec2 * cd1;
                                circOrigin = p1 - rotLvec2 * legCircleSize;
                                cv1 = normalize(legOuterCircle - circOrigin);
                                cv2 = normalize(p1 - circOrigin);
                                cvDiff1 = fPos - circOrigin;

                                c1Ang1 = atan(cv1.y, cv1.x);
                                c1Ang2 = atan(cv2.y, cv2.x);
                                fAng = atan(cvDiff1.y, cvDiff1.x);

                                if (angleBetween(c1Ang1, c1Ang2, fAng))
                                {
                                    dist = legCircleSize - length(cvDiff1);
                                }
                            }
                        }
                    }

                    FragColour = getColour(dist, lineSize, bodyColour, edgeColour);
                    
                    if (dist > 0.0) found = true;

                    break;
                }
            }
        }
        
        if (!found)
        {
            FragColour = vec4(0.0, 0.0, 0.0, 0.0);
        }
        
        return;
    }

    FragColour = vec4(0.0, 0.0, 0.0, 0.0);
}