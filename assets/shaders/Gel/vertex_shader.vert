#version 460 core

#define MULT 2

layout (location = 0) in vec2 vertex;

layout (location = 0) flat out ivec4 outGelData[16];
layout (location = 16) out vec2 wp;

layout (push_constant) uniform PC
{    
    vec4 fData; // gridSize, none, none, none
    ivec4 iData; // swapchainImage, gelType, numSquares, arrStart
    ivec4 iData2; // xSquareCount, ySquareCount, none, start1
} gelData;

layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 cameraMatrix;
    vec4 cameraPos;
    vec4 data;
} sharedUboData;

struct storageData
{
    vec4 posR; // posx, posy, radius, none
    vec4 color;
};

struct gelSpatialData
{
    ivec4 gelIndices[32];
};

layout (std140, set = 1, binding = 0) readonly buffer UBO
{
    storageData sData[];
} ubo;

layout (std140, set = 1, binding = 1) readonly buffer UBO2
{
    gelSpatialData sData[];
} ubo2;

void main()
{
    int instanceID = gl_InstanceIndex;
    int numxGrid = int(sharedUboData.cameraPos.z + 2) * MULT;
    int numyGrid = int(sharedUboData.cameraPos.w + 2) * MULT;
    
    int thisX = (instanceID % numxGrid - numxGrid / 2);
    int thisY = (instanceID / numxGrid - numyGrid / 2);

    vec2 center = vec2(thisX + 0.5, thisY + 0.5) / MULT + sharedUboData.cameraPos.xy * vec2(1.0, -1.0);

    int gelIndex = 0;

    int swapchainImage = gelData.iData[0];
    int gelType = gelData.iData[1];
    int numSquares = gelData.iData[2];
    int arrStart = gelData.iData[3];

    int xSquareCount = gelData.iData2[0];
    int ySquareCount = gelData.iData2[1];
    int start1 = gelData.iData2[3];

    vec2 arrV = center + vec2(xSquareCount, ySquareCount) - sharedUboData.cameraPos.xy * vec2(1.0, -1.0);

    for (int i = -1; i < 2; i++)
    {
        for (int j = -1; j < 2; j++)
        {
            int arrPos = int(arrV.x / 2.0) + i + (int(arrV.y / 2.0) + j) * xSquareCount + arrStart;
            if (arrPos < 0)
			{
				continue;
			}

            int numGels = ubo2.sData[arrPos].gelIndices[0][0];
            for (int k = 1; k <= numGels; k++)
            {
                int index = ubo2.sData[arrPos].gelIndices[k / 4][k % 4];
                vec4 p = ubo.sData[index + start1].posR;
                if (length(p.xy - center) - p.z < 0.75)
                {
                    outGelData[gelIndex / 4 + 1][gelIndex % 4] = index + start1;
                    gelIndex++;
                }
            }
        }
    }

    outGelData[0][0] = gelIndex;

    if (gelIndex == 0)
    {
        // discard
        gl_Position = vec4(0.0, 0.0, 0.0, 0.0);
    }
    else
    {
        wp = vertex / MULT + center;
        gl_Position = sharedUboData.cameraMatrix * vec4(wp, 0.0, 1.0);
    }
}