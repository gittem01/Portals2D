#version 460 core

layout (location = 0) out vec4 FragColour;

layout (location = 0) in vec2 localPos;
layout (location = 1) flat in vec2 size;
layout (location = 2) flat in vec4 wavColor;
layout (location = 3) flat in vec4 portal;
layout (location = 4) in vec2 worldPos;

layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 cameraMatrix;
    vec4 cameraPos;
    vec4 data;
} sharedUboData;

float funcHelper(float val)
{
    float mult2 = pow(val / size.x, 2.5) + 0.1f;
    val = (val / size.x) * 30.0f;

    float v = pow((30.0f - val) / 1.5f, 0.5f);

    return val * v * mult2 * 0.0075 * size.y;
}

float func1(float val, float secondVal, float thirdVal)
{
    float revVal = size.x - val;
    float retVal =  sin(+revVal * 0.37656 + secondVal * 360.0) * 0.299 + 
                    sin(-revVal * 0.16784 + thirdVal  * 220.0) * 0.263 +
                    sin(+revVal * 0.36784 + thirdVal  * 170.0) * 0.209;
    
    return retVal * funcHelper(val);
}

float func2(float val, float secondVal, float thirdVal)
{
    float revVal = size.x - val;
    float retVal =  sin(-revVal * 0.21256 + secondVal * 305.0) * 0.239 + 
                    sin(+revVal * 0.36634 + thirdVal  * 397.0) * 0.313 +
                    sin(-revVal * 0.36784 + thirdVal  * 271.0) * 0.309;
    
    return retVal * funcHelper(val);
}

float func3(float val, float secondVal, float thirdVal)
{
    float revVal = size.x - val;
    float retVal =  sin(-revVal * 0.36316 + secondVal * 174.0) * 0.321 + 
                    sin(+revVal * 0.16483 + thirdVal  * 296.0) * 0.176 +
                    sin(+revVal * 0.25718 + thirdVal  * 326.0) * 0.288;
 
    return retVal * funcHelper(val);
}

float lineRel(vec2 a, vec2 b, vec2 c)
{
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

void main()
{
    if (!isnan(portal.x) && lineRel(portal.xy, portal.zw, worldPos) > 0.0)
    {
        FragColour = vec4(0.0f, 0.0f, 0.0f, 0.0f);
        return;
    }

    vec2 uv = localPos;
    float iTime = sharedUboData.data.y;

    float r1 = func1(uv.x,
        (iTime * 1.67352) * 0.0163521,
        (iTime * 0.231532) * 0.013141);
        
    float r2 = func2(uv.x,
        (iTime * 1.27352) * 0.0183521,
        (iTime * 0.431532) * 0.019141);
        
    float r3 = func3(uv.x,
        (iTime * 1.93272) * 0.0116361,
        (iTime * 0.783232) * 0.014669);

    float l1 = length(uv - vec2(uv.x, r1 + size.y * 0.5));
    float l2 = length(uv - vec2(uv.x, r2 + size.y * 0.5));
    float l3 = length(uv - vec2(uv.x, r3 + size.y * 0.5));
    float minLen = min(min(l1, l2), l3);

    float num = 4.0;
    float sumLen =  max(pow((num - l1) / num, 3.0), 0.0) +
                    max(pow((num - l2) / num, 3.0), 0.0) +
                    max(pow((num - l3) / num, 3.0), 0.0);

    vec2 p = uv;
    vec2 diff = p - vec2(size.x - 3.0, 15.0f);

    if (sumLen < 0.001f) sumLen = 0.0f;

    float fracter = pow(uv.x / size.x, 2.0);

    float lastMult = 1.0f;
    if (uv.x > size.x - 5.0f)
    {
        lastMult = (size.x - uv.x) / 5.0f;
    }

    FragColour = sumLen * 2.0 * fracter * lastMult * wavColor;
}