#version 460 core

layout (location = 0) out vec4 FragColour;

layout (location = 0) in vec4 localPos;
layout (location = 1) in vec4 worldPos;
layout (location = 2) in vec2 tc;

layout (location = 3) in InData
{
    vec4 bgColour;
    vec4 dotColour;
    vec4 edgeColour;
    vec4 data; // dotSize, sideMargin, sideMult, smoothRatio
    vec4 data2; // sizex, sizey, camMult, unused
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
    float sideMargin = data.y * 4.0;
    float sideMult = data.z;
    float smoothRatio = data.w;

    float sizeX = data2.x;
    float sizeY = data2.y;

    float m = 1.0;
    if (worldPos.x > 5.0 && worldPos.x < 15.0 &&
        worldPos.y > 3.0 && worldPos.y < 7.0 || true)
    {
        const float pVal = 4.0;
        m = 0.0;
        m += pow(max(1.0 - (worldPos.x + 4.0), 0.0), pVal);
        m += pow(max(1.0 - (15.0 - worldPos.x), 0.0), pVal);
        m += pow(max(1.0 - (worldPos.y - 3.0), 0.0), pVal);
        m += pow(max(1.0 - (7.0 - worldPos.y), 0.0), pVal);
        m = min(m, 1.0);

        float adder = 0.02;
        m += adder;
        m *= 1.0 / (1.0 + adder);
    }

    vec4 txClr = texture(tex, tc);
    vec4 grdClr = texture(gradTex, tc / vec2(sizeX / dotSize, sizeY / dotSize));
    vec4 clr = vec4(vec3(dotColour) * txClr.w + vec3(bgColour) * (1.0 - txClr.w), 1.0);

    float divRat = 0.5;

    grdClr.x = (grdClr.x - (1.0 - 1.0 / 2.0)) * 2.0;
    grdClr.z = (grdClr.z - (1.0 - 1.0 / 4.0)) * 4.0;

    grdClr.x = max(grdClr.x, grdClr.z);

    if (grdClr.x < divRat && grdClr.x > 0.0)
    {
        float ratio = grdClr.x / divRat;
        ratio = pow(ratio, 1.0);
        FragColour = (clr * (1.0 - ratio) + bgColour * ratio) * m;
        return;
    }
    else if (grdClr.x > divRat)
    {
        float ratio = (grdClr.x - divRat) / (1 - divRat);
        ratio = pow(ratio, 2.0);
        FragColour = (bgColour * (1.0 - ratio) + edgeColour * ratio) * m;
    }
    else
    {
        FragColour = clr * m;
    }

    return;
}