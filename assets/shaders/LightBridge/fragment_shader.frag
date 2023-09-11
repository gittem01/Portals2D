#version 460 core

layout (location = 0) out vec4 FragColour;

layout (location = 0) in vec2 localPos;
layout (location = 1) flat in vec2 size;
layout (location = 2) flat in vec4 outData;
layout (location = 3) flat in vec4 portal;

layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 cameraMatrix;
    vec4 cameraPos;
    vec4 data;
} sharedUboData;

void main()
{
    vec4 outerColour = vec4(1.0, 1.0, 1.0, 1.0) * outData.z;
    vec4 innerColour = vec4(0.0, 0.4, 1.0, 0.65) * outData.z;
    
    float iTime = sharedUboData.data.y;
    float absY = abs(localPos.y);
    float ySize = size.y;

    float sidePos = 0.15;
    float innerSize = 0.05;

    float outMult;
    float inMult;
    if (absY > sidePos)
    {
        float diff = ySize - sidePos;
        float ratio = 1.0 - (absY - sidePos) / diff;
        outMult = ratio;
        inMult = 0.0;
    }
    else if (sidePos > absY && absY > sidePos - innerSize)
    {
        float ratio = 1.0 - (sidePos - absY) / innerSize;
        outMult = ratio;
        inMult = 1.0 - outMult;
    }
    else
    {
        outMult = 0.0;
        inMult = 1.0;
        
        float sinVal = sin(localPos.x * 30.0 - iTime * 9.0);
        float sinVal2 = sin(localPos.x * 20.0 - iTime * 3.1135);
        float sinAdd = max(0.5 - abs(localPos.y * 12.0 - sinVal * 0.5), 0.0);
        sinAdd += max(0.5 - abs(localPos.y * 15.0 - sinVal2 * 0.5), 0.0);
        sinAdd = smoothstep(0.0, 0.7, sinAdd);
        inMult -= sinAdd;
        outMult += sinAdd;
    }

    float endMult = 1.0;
    if (portal.x < -4567654333.00387156 && localPos.x > size.x - 0.15)
    {
        endMult = abs(size.x - localPos.x) / 0.15;
    }

    FragColour = (outerColour * outMult + innerColour * inMult) * endMult;
}