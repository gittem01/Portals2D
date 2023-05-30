#version 460 core

#define PI 3.14159265359
#define SPEED 0.03

layout (location = 0) out vec4 FragColour;

layout (location = 0) in vec2 localPos;
layout (location = 1) in vec2 size;
layout (location = 2) in vec3 color0;
layout (location = 3) in vec3 color1;
layout (location = 4) in float circleRadius;

layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 cameraMatrix;
    vec4 cameraPos;
    vec4 data;
} sharedUboData;

void main()
{
    float pixeller = 0.03;
    vec2 localPosCopy = localPos;
    localPosCopy.x = int(localPos.x / pixeller) * pixeller;
    localPosCopy.y = int(localPos.y / pixeller) * pixeller;

    vec2 absPos = abs(localPosCopy);
    vec2 sideCirclePos = vec2(size.x * 0.5 - size.y * 0.5, 0.0);
    float hyp = size.y * 0.5 + circleRadius;
    float bot = size.x * 0.5 - size.y * 0.5;
    float up = sqrt(hyp * hyp - bot * bot);
    float ang = atan(up, -bot);

    vec2 centerCirclePos = vec2(0.0, up);
    vec2 diffVec = centerCirclePos - absPos;
    float diff = size.y * 0.5 - length(diffVec) + circleRadius;
    float vecAngle = atan(diffVec.y, diffVec.x);

    if (length(absPos - sideCirclePos) < size.y * 0.5 ||
    (diff < size.y * 0.5 && vecAngle < ang && up > circleRadius) && absPos.y < up)
    {
        float val;
        if (vecAngle < ang && absPos.y < up && up > circleRadius)
        {
            val = diff;
        }
        else
        {
            val = length(absPos - sideCirclePos);
        }
    
        float xRatio = localPosCopy.x / size.x;
        xRatio = xRatio / 0.1 + 0.5;
        vec3 clr = xRatio * color0 + (1.0 - xRatio) * color1;
        float colorVal = 1.0 - val / (size.y * 1.0);
        colorVal = atan((colorVal - 0.5) * 5.0);
        FragColour = vec4(colorVal * clr, colorVal);
    }
    else
    {
        FragColour = vec4(0.0);
    } 
}