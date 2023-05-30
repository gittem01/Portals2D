#version 460 core

layout (location = 0) out vec4 FragColour;

layout (location = 0) in vec4 normal;
layout (location = 1) in vec4 localPos;
layout (location = 2) in vec4 worldPos;
layout (location = 3) in vec2 tc;

layout (location = 4) in InData
{
    vec4 bgColour;
    vec4 dotColour;
    vec4 edgeColour;
    vec4 leftUp;
    vec4 data;
    vec4 data2;
};

layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 cameraMatrix;
    vec4 cameraPos;
    vec4 data;
} sharedUboData;

layout(set = 2, binding = 0) uniform sampler2D tex;
layout(set = 2, binding = 1) uniform sampler2D gradTex;

void main()
{   
    float dotSize = data.x;
    dotSize = 1.0;
    float sideMargin = data.y;
    sideMargin = 0.15;
    float sideMult = data.z;
    float smoothRatio = data.w;
    smoothRatio = 0.1;
    float sizeX = data2.x;
    float sizeY = data2.y;

    vec4 localPos = localPos;
    float pixeller = 0.02;
    localPos.x = int(localPos.x / pixeller) * pixeller;
    localPos.y = int(localPos.y / pixeller) * pixeller;
    float sideDistCount = 3.0;
    float minSideDist = min(localPos.x, sizeX - localPos.x);
    minSideDist = min(localPos.y, minSideDist);
    minSideDist = min(sizeY - localPos.y, minSideDist);

    if (minSideDist < sideMargin * sideDistCount)
    {
        float blurAmount = min(minSideDist / sideMargin, 1.0);
        FragColour = edgeColour * (1.0 - blurAmount) + bgColour * blurAmount;
        return;
    }

    localPos.x -= sideDistCount * sideMargin;
    float newResX = sizeX - sideDistCount * sideMargin * 2.0;

    float requiredDotCountX = floor(newResX / dotSize);
    if (int(requiredDotCountX) % 2 == 0)
    {
        requiredDotCountX -= 1.0;
    }
    
    float xLeft = newResX - requiredDotCountX * dotSize;
    float perXLeftPerct = (xLeft / requiredDotCountX) / dotSize;
    
    localPos.y -= sideDistCount * sideMargin;
    float newResY = sizeY - sideDistCount * sideMargin * 2.0;

    float requiredDotCountY = floor(newResY / dotSize);
    if (int(requiredDotCountY) % 2 == 0)
    {
        requiredDotCountY -= 1.0;
    }
    
    float yLeft = newResY - requiredDotCountY * dotSize;
    float perYLeftPerct = (yLeft / requiredDotCountY) / dotSize;

    float minLeftPerct = min(perXLeftPerct, perYLeftPerct);
    float pDist2 = pow(0.5, 2.0);

    float xNorm = localPos.x / newResX;
    float yNorm = localPos.y / newResY;

    float xVal = mod(xNorm, 1.0 / requiredDotCountX);
    xVal *= requiredDotCountX;
    xVal -= 0.5;
    xVal *= (1.0 + perXLeftPerct);
    
    float xIndexPerct = floor(localPos.x / (newResX / requiredDotCountX)) / (requiredDotCountX - 1.0);
    xVal += perXLeftPerct * (0.5 - xIndexPerct);
    
    float yVal = mod(yNorm, 1.0 / requiredDotCountY);
    yVal *= requiredDotCountY;
    yVal -= 0.5;
    yVal *= (1.0 + perYLeftPerct);
    float yIndexPerct = floor(localPos.y / (newResY / requiredDotCountY)) / (requiredDotCountY - 1.0);
    yVal += perYLeftPerct * (0.5 - yIndexPerct);
    
    float xMask = mod(xNorm, 2.0 / requiredDotCountX);
    xMask *= requiredDotCountX;
    
    float yMask = mod(yNorm, 2.0 / requiredDotCountY);
    yMask *= requiredDotCountY;
    
    float dist2 = xVal * xVal + yVal * yVal;
    if (dist2 < pDist2 && ((xMask > 1.0 && yMask > 1.0) || (xMask < 1.0 && yMask < 1.0)))
    {
        float r = pow(2.0, -dotSize * 3);
        if (pDist2 - dist2 < pDist2 * r)
        {
            float ratio = ((pDist2 - dist2) / r) / pDist2;
            //ratio = pow(ratio, 2.0);
            FragColour = dotColour * ratio + bgColour * (1.0 - ratio);
        }
        else
        {
            FragColour = dotColour;
        }
    }
    else
    {
        FragColour = bgColour;
    }
}