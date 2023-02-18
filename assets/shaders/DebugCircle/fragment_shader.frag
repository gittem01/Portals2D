#version 460 core

layout (location = 0) out vec4 FragColour;

layout (location = 0) in vec4 localPos;
layout (location = 1) in vec2 worldPosition;

layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 orthoMatrix;
    vec4 data; // zoom, unused, unused, unused
} sharedUboData;

layout (std140, set = 1, binding = 0) readonly buffer UBO
{
    mat4 modelMatrix;
    vec4 portals[4];
    vec4 colour;
    vec4 outerColour;
    vec4 data; // numPortals, lineThickness, outerSmoothThicknessRatio, radius
    vec4 extraData; // offsetx, offsety, unused, unused;
} ubo;

float lineRel(vec2 a, vec2 b, vec2 c)
{
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

void main()
{
    int numPortals = int(round(ubo.data.x));
    for (int i = 0; i < numPortals; i++)
    {
        if (lineRel(ubo.portals[i].xy, ubo.portals[i].zw, worldPosition) > 0){
            FragColour = vec4(0.0f, 0.0f, 0.0f, 0.0f);
            return;
        }
    }

    float adjustedOuterLineWidth = ubo.data.y / (sharedUboData.data.x * sharedUboData.data.x * ubo.data.w);
    float len = length(localPos.xy - ubo.extraData.xy) / ubo.data.w;
    if (len < 1.0f - adjustedOuterLineWidth)
    {
        FragColour = ubo.colour;
    }
    if (len < 1.0f)
    {
        float midDist = 1 - adjustedOuterLineWidth * 0.5f;
        float distFromMidDist = len - midDist;
        if (len >= midDist)
        {
            float perct = distFromMidDist / (adjustedOuterLineWidth * 0.5f);
            float reverseOuter = ubo.data.z;
            if (perct > reverseOuter)
            {
                perct = (perct - reverseOuter) * (1.0f / (1.0f - ubo.data.z));
                FragColour = vec4(ubo.outerColour.xyz, (1 - perct) * ubo.outerColour.w);
            }
            else
            {
                FragColour = ubo.outerColour;
            }
        }
        else if (len >= 1 - adjustedOuterLineWidth)
        {
            float perct = -distFromMidDist / (adjustedOuterLineWidth * 0.5f);
            float reverseOuter = ubo.data.z;
            if (perct > reverseOuter)
            {
                perct = (perct - reverseOuter) * (1.0f / (1.0f - ubo.data.z));
                float alphaVal = ubo.outerColour.w + perct * (ubo.colour.w - ubo.outerColour.w);
                vec3 colourMix = ubo.outerColour.xyz * (1 - perct * ubo.colour.w / alphaVal) + ubo.colour.xyz * perct * ubo.colour.w / alphaVal;
                FragColour = vec4(colourMix, alphaVal);
            }
            else
            {
                FragColour = ubo.outerColour;
            }
        }
    }
    else
    {
        FragColour = vec4(0, 0, 0, 0);
    }
}