#version 460 core

#define PI 3.14159265359

layout (location = 0) out vec4 FragColour;

layout (location = 0) in vec2 localPos;
layout (location = 1) in vec2 worldPos;
layout (location = 2) flat in vec2 size;
layout (location = 3) flat in int instanceIndex;

layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 cameraMatrix;
    vec4 cameraPos;
    vec4 data;
} sharedUboData;

struct storageData
{
    vec4 pointts;
    vec4 colour;
    vec4 colour2;
    vec4 data; // thickness, offsetX, startX, colourInd
    vec4 data2; // splitX, endSmoothPosX, totalX, splitTime
};

layout (std140, set = 1, binding = 0) readonly buffer UBO
{
    storageData sData[];
} ubo;

float hash(ivec2 p)
{
    int n = p.x*3 + p.y*113;

	n = (n << 13) ^ n;
    n = n * (n * n * 15731 + 789221) + 1376312589;
    return -1.0 + 2.0 * float(n & 0x0fffffff) / float(0x0fffffff);
}

float noise(vec2 p)
{
    ivec2 i = ivec2(floor(p));
    vec2 f = fract(p);

    vec2 u = f * f * (3.0 - 2.0 * f);

    return mix(mix(hash(i + ivec2(0, 0)), 
                   hash(i + ivec2(1, 0)), u.x),
               mix(hash(i + ivec2(0, 1)), 
                   hash(i + ivec2(1, 1)), u.x), u.y);
}

float getNoise()
{
    mat2 m = mat2( 1.6,  1.2, -1.2,  1.6 );
    float f = 0.0;
    float sv = 0.5;
    int mx = 5;
    vec2 uv = worldPos + vec2(sharedUboData.data.y * 0.5, sharedUboData.data.y * 0.363);
    uv *= 4.0;
    for (int i = 0; i < mx; i++)
    {
        f += sv * noise(uv);
        uv = m * uv;
        sv /= 2.0;
    }

    return 0.8 + 0.2 * f;
}

float getClosestDistance(vec2 point, float midX, int iterations)
{
    float t = 0.5;
    float closestDistance = 1e30;

    for (int i = 0; i < iterations; i++)
    {
        float x = mix(midX - 0.1, midX + 0.1, t);
        float y = cos(x);
        float dx = point.x - x;
        float dy = point.y - y;
        float slope = sin(x);

        t += (dx * slope + dy) / (slope * slope + 1.0);

        t = clamp(t, 0.0, 1.0);

        closestDistance = min(closestDistance, length(vec2(dx, dy)));
    }

    return closestDistance;
}

float fromToFunc(float x, float from, float to)
{
    float diff = to - from;
    float sinVal = (sin((x - 0.5) * PI) + 1.0) * 0.5;
    return from + diff * sinVal;
}

void main()
{
    storageData data = ubo.sData[instanceIndex];

    float iTime = sharedUboData.data.y;

    float xFromStart = localPos.x + data.data.z + size.x * 0.5;
    vec2 p = vec2(xFromStart, localPos.y);
    p.x += (iTime - data.data2.w) * data.data.w + data.data.y;
    float yMult = 1.0 - abs(p.y);
    float endMult = 1.0;

    if (xFromStart > data.data2.y)
    {
        float v = data.data2.z - data.data2.y;
        endMult = 1.0 - (xFromStart - data.data2.y) / v;
    }

    const float colourSmoother = 5.0;
    vec4 clr;
    float colorSplitPos = data.data2.x + data.data.w * (iTime - data.data2.w) * -200.0;
    if (xFromStart > colorSplitPos)
    {
        float m = min(((xFromStart - colorSplitPos) / colourSmoother + 1.0) * 0.5, 1.0);
        clr = data.colour2 * m + data.colour * (1.0 - m);
    }
    else
	{
        float m = min(((colorSplitPos - xFromStart) / colourSmoother + 1.0) * 0.5, 1.0);
		clr = data.colour * m + data.colour2 * (1.0 - m);
	}

    float ln;
    float thick = 0.5;
    float sumLn = 0.0;
    for (int i = 0; i < 3; i++)
    {
        float perIVal0 = sin(float(i) * 2.0 + iTime);
        float perIVal1 = sin(float(i) * 1.7324 + iTime * 1.5);
        float extraMult;
    
        ln = getClosestDistance(p, p.x, 3);
        float aCos;
        float modVal = mod(p.x + perIVal0 * 0.3, 2.0 * PI);
        if (modVal > PI)
        {
            aCos = acos(p.y);
            extraMult = 1.0;
        }
        else
        {
            aCos = acos(-p.y);
            extraMult = 0.5;
        }
        ln = abs(sin(aCos - modVal));

        float thickCopy = thick + perIVal1 * 0.2;
        sumLn += max(pow(max(thickCopy - ln, 0.0) / thickCopy, 5.0), 0.0) * extraMult;
        
        p.x += PI * 2.0 / 3.0;
    }

    const float sideLineThick = 0.1;

    float lastVal = 0.0;
    if (sideLineThick - yMult > 0.0)
    {
        float newY = yMult > sideLineThick * 0.5 ? sideLineThick - yMult : yMult;
        lastVal = 1.0 - abs(sideLineThick * 0.5 - newY) / (sideLineThick * 0.5);
    }
    
    if (yMult > sideLineThick * 0.5)
    {
        lastVal += pow(1.0 - yMult, 4.0);
    }

    lastVal = max(min(lastVal, 1.0), 0.0);
       
    FragColour = clr * (sumLn * yMult + lastVal) * endMult;
}