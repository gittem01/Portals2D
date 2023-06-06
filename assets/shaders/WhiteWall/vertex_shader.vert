#version 460 core

layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 tc;

layout (location = 0) out vec4 outNormal;
layout (location = 1) out vec4 localPos;
layout (location = 2) out vec4 worldPos;
layout (location = 3) out vec2 out_tc;

layout (location = 4) out OutData
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

struct storageData
{
    mat4 transformMatrix;
    mat4 rotationMatrix;
    vec4 bgColour;
    vec4 dotColour;
    vec4 edgeColour;
    vec4 leftUp;
    vec4 data; // dotSize, sideMargin, sideMult, smoothRatio
    vec4 data2; // sizeX, sizeY, unused, unused
};

layout (std140, set = 1, binding = 0) readonly buffer UBO
{
    storageData sData[];
} ubo;

void main()
{
    int instanceID = gl_InstanceIndex;

    bgColour = ubo.sData[instanceID].bgColour;
    dotColour = ubo.sData[instanceID].dotColour;
    edgeColour = ubo.sData[instanceID].edgeColour;
    leftUp = ubo.sData[instanceID].leftUp;
    data = ubo.sData[instanceID].data;
    data2 = ubo.sData[instanceID].data2;
    
    vec4 scaledVertex = vec4(vertex * vec3(ubo.sData[instanceID].data2.xy, 1.0), 1.0);
    vec4 pos = sharedUboData.cameraMatrix * ubo.sData[instanceID].transformMatrix * ubo.sData[instanceID].rotationMatrix * scaledVertex;

    out_tc = tc * vec2(data2.x / data.x, data2.y / data.x);
    outNormal = ubo.sData[instanceID].rotationMatrix * vec4(normal, 1.0);
    localPos = vec4((vertex + vec3(0.5, 0.5, 0.0)) * vec3(ubo.sData[instanceID].data2.xy, 1.0), 1.0);
    worldPos = ubo.sData[instanceID].transformMatrix * ubo.sData[instanceID].rotationMatrix * scaledVertex;
    gl_Position = pos;
}