#version 460 core

layout (location = 0) out vec4 FragColour;

layout (location = 0) in vec2 localPos;
layout (location = 1) flat in vec2 size;
layout (location = 2) flat in vec4 outData;

layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 cameraMatrix;
    vec4 cameraPos;
    vec4 data;
} sharedUboData;

void main()
{
    vec4 outerColour = vec4(1.0, 1.0, 1.0, 1.0) * outData.z;
    vec4 innerColour = vec4(0.0, 0.4, 1.0, 0.2) * outData.z;
    
    float iTime = sharedUboData.data.y;
    float absY = abs(localPos.y);
    float ySize = size.y;

    float sidePos = 0.15;
    float innerSize = 0.05;

    float xPos = outData.w + localPos.x;

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
        
        float startMult = min(xPos, 2.0) * 0.5;
        float sinVal = sin(xPos * 10.0 - iTime * 7.0) + sin(xPos * 13.1 - iTime * 5.0);
        float sinVal2 = sin(xPos * 7.0 - iTime * 3.1135) + sin(xPos * 8.1 - iTime * 2.0);
        float sinAdd = max(0.5 - abs(localPos.y * 12.0 - sinVal * 0.4 * startMult), 0.0);
        sinAdd += max(0.5 - abs(localPos.y * 15.0 - sinVal2 * 0.4 * startMult), 0.0);
        sinAdd = smoothstep(0.0, 0.5, sinAdd);
        inMult -= sinAdd;
        outMult += sinAdd;
    }

    float endMult = 1.0;
    if (xPos > outData.y - 0.15)
    {
        endMult = abs(outData.y - xPos) / 0.15;
    }

    FragColour = (outerColour * outMult + innerColour * inMult) * endMult;
    //FragColour = vec4(1.0, 0.0, 0.0, 1.0);
}