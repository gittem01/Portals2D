#version 460 core

layout (location = 0) out vec4 FragColour;

layout (location = 0) in vec4 outNormal;
layout (location = 1) in vec4 localPos;

layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 cameraMatrix;
    vec4 cameraPos;
    vec4 data;
} sharedUboData;

layout (std140, set = 1, binding = 0) readonly buffer UBO
{
    mat4 transformMatrix;
    mat4 rotationMatrix;
    vec4 bgColour;
    vec4 dotColour;
    vec4 edgeColour;
    vec4 leftUp;
    vec4 data; // dotSize, sideMargin, sideMult, smoothRatio
    vec4 data2; // sizeX, sizeY, unused, unused
} ubo;

void main()
{
    float dotSize = ubo.data.x;

    float minSideDist = min(localPos.x, ubo.data2.x - localPos.x);
    minSideDist = min(localPos.y, minSideDist);
    minSideDist = min(ubo.data2.y - localPos.y, minSideDist);

    if (minSideDist < ubo.data.y * ubo.data.z)
    {
        float blurAmount = min(minSideDist / ubo.data.y, 1.0);
        FragColour = ubo.edgeColour * (1.0 - blurAmount) + ubo.bgColour * blurAmount;
        return;
    }

    vec2 localPosCopy = localPos.xy;

    localPosCopy.x -= ubo.data.z * ubo.data.y;
    float newResX = ubo.data2.x - ubo.data.z * ubo.data.y * 2.0;

    float requiredDotCountX = floor(newResX / dotSize);
    if (int(requiredDotCountX) % 2 == 0)
    {
        requiredDotCountX -= 1.0;
    }
    
    float xLeft = newResX - requiredDotCountX * dotSize;
    float perXLeftPerct = (xLeft / requiredDotCountX) / dotSize;
    
    localPosCopy.y -= ubo.data.z * ubo.data.y;
    float newResY = ubo.data2.y - ubo.data.z * ubo.data.y * 2.0;

    float requiredDotCountY = floor(newResY / dotSize);
    if (int(requiredDotCountY) % 2 == 0)
    {
        requiredDotCountY -= 1.0;
    }
    
    float yLeft = newResY - requiredDotCountY * dotSize;
    float perYLeftPerct = (yLeft / requiredDotCountY) / dotSize;

    float minLeftPerct = min(perXLeftPerct, perYLeftPerct);
    float pDist2 = pow(0.5 + minLeftPerct * 0.0, 2.0);

    float xNorm = localPosCopy.x / newResX;
    float yNorm = localPosCopy.y / newResY;

    float xVal = mod(xNorm, 1.0 / requiredDotCountX);
    xVal *= requiredDotCountX;
    xVal -= 0.5;
    xVal *= (1.0 + perXLeftPerct);
    
    float xIndexPerct = floor(localPosCopy.x / (newResX / requiredDotCountX)) / (requiredDotCountX - 1.0);
    xVal += perXLeftPerct * (0.5 - xIndexPerct);
    
    float yVal = mod(yNorm, 1.0 / requiredDotCountY);
    yVal *= requiredDotCountY;
    yVal -= 0.5;
    yVal *= (1.0 + perYLeftPerct);
    float yIndexPerct = floor(localPosCopy.y / (newResY / requiredDotCountY)) / (requiredDotCountY - 1.0);
    yVal += perYLeftPerct * (0.5 - yIndexPerct);
    
    float xMask = mod(xNorm, 2.0 / requiredDotCountX);
    xMask *= requiredDotCountX;
    
    float yMask = mod(yNorm, 2.0 / requiredDotCountY);
    yMask *= requiredDotCountY;
    
    float dist2 = xVal * xVal + yVal * yVal;
    if (dist2 < pDist2 && ((xMask > 1.0 && yMask > 1.0) || (xMask < 1.0 && yMask < 1.0)))
    {
        float r = 0.5f;
        if (pDist2 - dist2 < pDist2 * r)
        {
            float ratio = ((pDist2 - dist2) / r) / pDist2;
            FragColour = ubo.dotColour * ratio + ubo.bgColour * (1.0 - ratio);
        }
        else
        {
            FragColour = ubo.dotColour;
        }
    }
    else
    {
        FragColour = ubo.bgColour;
    }
}