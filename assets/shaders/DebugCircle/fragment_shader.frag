#version 460 core

layout (location = 0) out vec4 FragColour;

layout (location = 0) in vec4 localPos;
layout (location = 1) in vec2 worldPosition;

layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 orthoMatrix;
    vec4 cameraPos;
    vec4 data; // zoom, unused, unused, unused
} sharedUboData;

layout (std140, set = 1, binding = 0) readonly buffer UBO
{
    mat4 modelMatrix;
    vec4 portals[4];
    vec4 colour;
    vec4 outerColour;
    vec4 data; // numPortals, lineThickness, outerSmoothThicknessRatio, radius
    vec4 extraData; // offsetx, offsety, drawReleases, unused;
} ubo;

// drawReleases -2: fully released, -1: still colliding

float lineRel(vec2 a, vec2 b, vec2 c)
{
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

void main()
{
    int numPortals = int(round(ubo.data.x));
    vec4 colour = ubo.colour;
    vec4 outerColour = ubo.outerColour;

    if (ubo.extraData.z > -1.5f)
    {
        for (int i = 0; i < numPortals; i++)
        {
            if (lineRel(ubo.portals[i].xy, ubo.portals[i].zw, worldPosition) > 0)
            {
                if (ubo.extraData.z < -0.5f)
                {
                    colour = vec4(1.0f, 1.0f, 1.0f, 0.05f);
                    outerColour = vec4(1.0f, 1.0f, 1.0f, 0.8f);
                    break;
                }
                else
                {
                    FragColour = vec4(0.0f, 0.0f, 0.0f, 0.0f);
                    return;
                }
            }
        }
    }
    else
    {
        colour = vec4(1.0f, 1.0f, 1.0f, 0.05f);
        outerColour = vec4(1.0f, 1.0f, 1.0f, 0.4f);
    }

    float adjustedOuterLineWidth = ubo.data.y / (sharedUboData.data.x * sharedUboData.data.x * ubo.data.w);
    float len = length(localPos.xy - ubo.extraData.xy) / ubo.data.w;
    if (len < 1.0f - adjustedOuterLineWidth)
    {
        FragColour = colour;
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
                FragColour = vec4(outerColour.xyz, (1 - perct) * outerColour.w);
            }
            else
            {
                FragColour = outerColour;
            }
        }
        else if (len >= 1 - adjustedOuterLineWidth)
        {
            float perct = -distFromMidDist / (adjustedOuterLineWidth * 0.5f);
            float reverseOuter = ubo.data.z;
            if (perct > reverseOuter)
            {
                perct = (perct - reverseOuter) * (1.0f / (1.0f - ubo.data.z));
                float alphaVal = outerColour.w + perct * (colour.w - outerColour.w);
                vec3 colourMix = outerColour.xyz * (1 - perct * colour.w / alphaVal) + colour.xyz * perct * colour.w / alphaVal;
                FragColour = vec4(colourMix, alphaVal);
            }
            else
            {
                FragColour = outerColour;
            }
        }
    }
    else
    {
        FragColour = vec4(0, 0, 0, 0);
    }
}