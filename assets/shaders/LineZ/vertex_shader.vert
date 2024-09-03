#version 460 core

layout (location = 0) in vec2 vertex;

layout (location = 0) out vec2 localPos;
layout (location = 1) flat out vec2 size;
layout (location = 2) flat out vec4 colour;

layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 orthoMatrix;
    vec4 cameraPos;
    vec4 data; // zoom, unused, unused, unused
} sharedUboData;

struct storageData
{
    vec4 pointts; // frontX, frontY, backX, backY
    vec4 colour;
    vec4 data; // thickness, zmult0, zmult1, none
};

layout (std140, set = 1, binding = 0) readonly buffer UBO
{
    storageData sData[];
} ubo;

int vertexTypes = 0x1C;

void main() 
{
	// get vertex id
    int vId = gl_VertexIndex;
    
    // 0, 1, 5 : back 2, 3, 4 : front
    int vertexType = (vertexTypes & (1 << vId)) > 0 ? 1 : 0;
    int iId = gl_InstanceIndex;
    storageData data = ubo.sData[iId];

    colour = data.colour;

    vec2 ps[2];
    ps[0] = ubo.sData[iId].pointts.zw;
    vec2 mcp = vec2(sharedUboData.cameraPos.x, -sharedUboData.cameraPos.y);
    vec2 camPosDiff = ps[0] - mcp;
    ps[0] = mcp + vec2(camPosDiff.x, camPosDiff.y) * data.data.y;

    ps[1] = ubo.sData[iId].pointts.xy;
    camPosDiff = ps[1] - mcp;
    ps[1] = mcp + vec2(camPosDiff.x, camPosDiff.y) * data.data.z;

    size = vec2(data.data.x, length(ps[1] - ps[0]));
    localPos = vertex * size;

    vec2 diff = ps[0] - ps[1];
    vec2 rightVec = normalize(vec2(diff.y, -diff.x));

    vec2 p = ps[vertexType] + vertex.x *  rightVec * data.data.x;

	gl_Position = sharedUboData.orthoMatrix * vec4(p, 0.0, 1.0);
}