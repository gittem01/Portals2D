#version 460 core

#define PI 3.14159265359
#define SPEED 0.03

layout (location = 0) out vec4 FragColour;

layout (location = 0) in vec2 localPos;

layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 cameraMatrix;
    vec4 cameraPos;
    vec4 data;
} sharedUboData;

layout (std140, set = 1, binding = 0) uniform UBO
{
    vec4 posSize; // pos.x, pos.y, size.x, size.y
    vec4 color;
    vec4 data0; // dirAngle, pixeler, midThick, xDropSize
} ubo;

float editorFunc(float val)
{
    float lastMult;
    if (val < 0.0) lastMult = -1.0f;
    else lastMult = 1.0f;
    return pow(val, 0.8) * 2.9 * lastMult;
}

void main()
{
    float xSize = ubo.posSize.z;
    float ySize = ubo.posSize.w;
 
    float xRatio = localPos.x / xSize;
    if (ubo.color.w > 0.5) xRatio *= -1.0f;
    vec3 color = ubo.color.xyz + vec3(1.0) * xRatio * 0.75;

    float pixeller = ubo.data0.y;

    vec2 pixelledPos = localPos;
    pixelledPos.x = int(pixelledPos.x / pixeller) * pixeller;
    pixelledPos.y = int(pixelledPos.y / pixeller) * pixeller;

    // ySize plus
    float midThick = ubo.data0.z;
    float xDropSize = max(ubo.data0.w, ySize * 0.5);
        
    float a = ySize * 0.5;
    float b = ySize * 0.5 + xDropSize;
    float c = midThick;
    float x = (b * b + c * c - a * a) / (2.0 * a - 2.0 * c);
    float angle = atan(x, b);
    float extraMid = cos(angle) * a;
    vec2 upCircPos = vec2(xSize * 0.5 - (a + b), x + c);
    if (abs(pixelledPos.x) > xSize * 0.5 - ySize * 0.5 - extraMid)
    {
        float xDiff = abs(pixelledPos.x) - (xSize * 0.5 - ySize * 0.5);
        float yDiff = abs(pixelledPos.y);
        float fullDiff = 1.0 - sqrt(xDiff * xDiff + yDiff * yDiff) / (ySize * 0.5);
        vec2 p = vec2(abs(pixelledPos.x), abs(pixelledPos.y));
        float diff = (length(upCircPos - p) - x) / (ySize * 0.5);
        vec2 diffVec = p - vec2(xSize * 0.5 - ySize * 0.5, 0.0);
        float ang2 = atan(diffVec.y, diffVec.x);
        if (PI - ang2 < angle)
        {
            fullDiff = diff;
        }
        
        float multVal = editorFunc(fullDiff);
        FragColour = vec4(color * multVal, multVal);
        return;
    }
    else if (abs(pixelledPos.x) > xSize * 0.5 - ySize - xDropSize)
    {
        vec2 p = vec2(abs(pixelledPos.x), abs(pixelledPos.y));
        float diff = (length(upCircPos - p) - x) / (ySize * 0.5);
        float multVal = editorFunc(diff);
        FragColour = vec4(color * multVal, multVal);
        return;
    }
    else if (abs(pixelledPos.y) < midThick)
    {
        float diff = ((midThick - abs(pixelledPos.y)) * (midThick / (ySize * 0.5))) / midThick;
        float multVal = editorFunc(diff);
        FragColour = vec4(color * multVal, multVal);
        return;
    }
    else
    {
        FragColour = vec4(0.0, 0.0, 0.0, 0.0);
    }
}