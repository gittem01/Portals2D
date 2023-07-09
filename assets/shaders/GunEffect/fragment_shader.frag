#version 460 core

layout (location = 0) out vec4 FragColour;

layout (location = 0) in vec2 localPos;
layout (location = 1) flat in vec2 size;

layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 cameraMatrix;
    vec4 cameraPos;
    vec4 data;
} sharedUboData;

const float upMult = 0.003;

float func1(float val, float secondVal, float thirdVal)
{
    
    float retVal =  sin(+val * 0.87656 + secondVal * 360.0) * 0.299 + 
                    sin(-val * 0.56784 + thirdVal  * 220.0) * 0.263 +
                    sin(+val * 0.96784 + thirdVal  * 170.0) * 0.209;
    
    const float m = 5.0f * (3.0f / 10.0f);
    float v = (30.0f - val) / m;
    v = pow(v, 0.3f);
    val *= v;
    val = pow(val, 2.2);
    
    return retVal * val * upMult;
}

float func2(float val, float secondVal, float thirdVal)
{
    
    float retVal =  sin(-val * 0.51256 + secondVal * 305.0) * 0.239 + 
                    sin(+val * 0.86634 + thirdVal  * 397.0) * 0.313 +
                    sin(-val * 0.76784 + thirdVal  * 271.0) * 0.309;
    
    const float m = 5.0f * (3.0f / 10.0f);
    float v = (30.0f - val) / m;
    v = pow(v, 0.3f);
    val *= v;
    val = pow(val, 2.2);
    
    return retVal * val * upMult;
}

float func3(float val, float secondVal, float thirdVal)
{
    
    float retVal =  sin(-val * 0.96316 + secondVal * 174.0) * 0.321 + 
                    sin(+val * 0.36483 + thirdVal  * 296.0) * 0.176 +
                    sin(+val * 0.65718 + thirdVal  * 326.0) * 0.288;
    
    const float m = 5.0f * (3.0f / 10.0f);
    float v = (30.0f - val) / m;
    v = pow(v, 0.3f);
    val *= v;
    val = pow(val, 2.2);
    
    return retVal * val * upMult;
}

void main()
{
    vec4 wavColor = vec4(0.0, 0.5, 1.0, 1.0);
    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = localPos;
    float iTime = sharedUboData.data.y;

    float r1 = func1((uv.x / size.x) * 30.0,
        (iTime * 1.67352) * 0.0163521,
        (iTime * 0.231532) * 0.013141);
        
    float r2 = func2((uv.x / size.x) * 30.0,
        (iTime * 1.27352) * 0.0183521,
        (iTime * 0.431532) * 0.019141);
        
    float r3 = func3((uv.x / size.x) * 30.0,
        (iTime * 1.93272) * 0.0116361,
        (iTime * 0.783232) * 0.014669);

    float l1 = length(uv - vec2(uv.x, r1 + size.y * 0.5));
    float l2 = length(uv - vec2(uv.x, r2 + size.y * 0.5));
    float l3 = length(uv - vec2(uv.x, r3 + size.y * 0.5));
    float minLen = min(min(l1, l2), l3);

    float num = 5.0;
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

    FragColour = vec4(sumLen * 2.0) * fracter * lastMult * wavColor;
}