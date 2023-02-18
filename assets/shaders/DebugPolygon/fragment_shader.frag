#version 460 core

layout (location = 0) out vec4 color;

layout (location = 0) in vec2 localPos;
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
    vec4 extraData;
} ubo;

layout (std140, set = 1, binding = 1) readonly buffer UBO_POLY
{
    vec4 polyPoints[4];
    int numPolyPoints;
} polyData;

float getDist(vec2 a, vec2 b, vec2 c)
{
    float v1 = (b.x - a.x) * (a.y - c.y);
    float v2 = (a.x - c.x) * (b.y - a.y);
    float top = abs(v1 - v2);

    v1 = pow(b.x - a.x, 2);
    v2 = pow(b.y - a.y, 2);
    float bot = sqrt(v1 + v2);

    return top / bot;
}

float lineRel(vec2 a, vec2 b, vec2 c)
{
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

float minVal = 54426262.736f;

void main()
{
    int numPortals = int(round(ubo.data.x));
    float zoom = sharedUboData.data.x;
    color = vec4(1.0f, 0.0f, 0.0f, 1.0f);
    for (int i = 0; i < numPortals; i++)
    {
        if (lineRel(ubo.portals[i].xy, ubo.portals[i].zw, worldPosition) > 0){
            color = vec4(0.0f, 0.0f, 0.0f, 0.0f);
            return;
        }
    }

    vec4 innerColour = ubo.colour;
    float d = ubo.data.y / (zoom * zoom);
    
    for (int i1 = 0; i1 < polyData.numPolyPoints; i1++)
    {
        int i2 = (i1 + 1) % polyData.numPolyPoints;

        vec2 v1 = vec2(polyData.polyPoints[i1 / 2][(i1 % 2) * 2], polyData.polyPoints[i1 / 2][(i1 % 2) * 2 + 1]);
        vec2 v2 = vec2(polyData.polyPoints[i2 / 2][(i2 % 2) * 2], polyData.polyPoints[i2 / 2][(i2 % 2) * 2 + 1]);
        float dist = getDist(v1, v2, localPos);
        if (dist < d)
        {
            if (dist < minVal)
            {
                minVal = dist;
            }
        }
    }

    if (minVal != 54426262.736f)
    {
        float bias = ubo.data.z;
        float perct = (d * 0.5f - minVal) / (d * 0.5f);
        if (perct > bias)
        {
            float newPerct = (perct - bias) / (1 - bias);
            color = vec4(ubo.outerColour.xyz, (1 - newPerct) * ubo.outerColour.w);
        }
        else if (perct < -bias)
        {
            float newPerct = (-perct - bias) / (1 - bias);
            float alphaVal = ubo.outerColour.w + newPerct * (innerColour.w - ubo.outerColour.w);
            vec3 colourMix = ubo.outerColour.xyz * (1 - newPerct * innerColour.w / alphaVal) + innerColour.xyz * newPerct * innerColour.w / alphaVal;
            color = vec4(colourMix, alphaVal);
        }
        else
        {
            color = ubo.outerColour;
        }
    }
    else
    {
        color = innerColour;
    }
}