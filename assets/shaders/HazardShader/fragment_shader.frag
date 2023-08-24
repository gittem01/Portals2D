#version 460 core

#define PI 3.14159265359

layout (location = 0) out vec4 FragColour;

layout (location = 0) in vec2 localPos;
layout (location = 1) in vec2 worldPos;
layout (location = 2) flat in vec2 size;
layout (location = 3) flat in vec4 colour;
layout (location = 4) flat in vec4 data;

layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 cameraMatrix;
    vec4 cameraPos;
    vec4 data;
} sharedUboData;

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

float func1(float val, float secondVal, float thirdVal)
{
    
    float retVal =  sin(+val * 0.87656 + secondVal * 360.0) * 0.299 + 
                    sin(-val * 0.56784 + thirdVal  * 220.0) * 0.263 +
                    sin(+val * 0.96784 + thirdVal  * 170.0) * 0.209;
    
    return retVal;
}

void main()
{
    const float smoothSize = min(size.x * 0.1, 5.0);

    float xSize = size.x;
    float yMax = size.y * 0.5;
    float iTime = sharedUboData.data.y;

    float mm = 40.0 / size.y;
    float ySizeMult = 0.03 * min(size.x * 0.5 - abs(localPos.x), 1.0) * size.y;
    
    float r1 = func1(localPos.x * mm,
        (iTime * 1.67352) * 0.0163521,
        (iTime * 0.231532) * 0.013141) * ySizeMult;
        
    float r2 = func1(localPos.x * mm,
        (-iTime * 1.27352) * 0.0183521,
        (iTime * 0.431532) * 0.019141) * ySizeMult;
        
    float r3 = func1(localPos.x * mm,
        (iTime * 1.93272) * 0.0116361,
        (iTime * 0.783232) * 0.014669) * ySizeMult;

    float l1 = length(localPos - vec2(localPos.x, r1 + size.y * 0.33));
    float l2 = length(localPos - vec2(localPos.x, r2));
    float l3 = length(localPos - vec2(localPos.x, r3 - size.y * 0.33));
    float minLen = min(min(l1, l2), l3);

    float num = size.y * 0.55;
    float sumLen =  max(pow((num - l1) / num, 3.0), 0.0) +
                    max(pow((num - l2) / num, 3.0), 0.0) +
                    max(pow((num - l3) / num, 3.0), 0.0);

    float v = pow(sumLen, 3.0);
    
    float timeDiff = (sharedUboData.data.y - data.w) * min(200.0, size.x * 5.0);
    float splitX = (-smoothSize + timeDiff) * data.z + (size.x * 0.5 + smoothSize - timeDiff) * (1.0 - data.z);

    v *= min(max(splitX - (size.x * 0.5 - abs(localPos.x)), 0.0), smoothSize) / smoothSize;

    const float sideLineThick = 0.06 / size.y;
    float yMult = 0.5 - abs(localPos.y) / size.y;

    FragColour = v * colour;
}