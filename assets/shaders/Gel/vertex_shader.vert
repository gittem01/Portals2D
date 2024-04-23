#version 460 core

layout (location = 0) in vec2 vertex;

layout (location = 0) flat out ivec4 outGelData[16];
layout (location = 16) out vec2 wp;

layout (push_constant) uniform PC
{    
    vec4 color;
    vec4 fData; // gridSize, none, none, none
    ivec4 iData; // numGridX, numGridY, numGels, none
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
};

layout (std140, set = 1, binding = 0) readonly buffer UBO
{
    storageData sData[];
} ubo;

void main()
{
    int instanceID = gl_InstanceIndex;
    int numxGrid = int(sharedUboData.cameraPos.z + 2);
    int numyGrid = int(sharedUboData.cameraPos.w + 2);
    
    int thisX = instanceID % numxGrid - numxGrid / 2;
    int thisY = instanceID / numxGrid - numyGrid / 2;

    vec2 center = vec2(thisX + 0.5, thisY + 0.5) + sharedUboData.cameraPos.xy * vec2(1.0, -1.0);

    int gelIndex = 0;
    for (int i = 0; i < 1024; i++)
    {
        vec4 p = ubo.sData[i].posR;
        if (length(p.xy - center) - p.z < 0.9)
        {
            outGelData[gelIndex / 4 + 1][gelIndex % 4] = i;
            gelIndex++;
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
        vec2 pos = vertex + center;
        wp = pos;
        gl_Position = sharedUboData.cameraMatrix * vec4(pos, 0.0, 1.0);
    }
}