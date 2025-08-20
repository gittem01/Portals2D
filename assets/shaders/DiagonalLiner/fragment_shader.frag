#version 460 core

#define PI 3.14159265359

layout (location = 0) out vec4 FragColour;

layout (location = 0) in vec2 localPos;
layout (location = 1) in vec2 worldPos;
layout (location = 2) flat in vec4 lineColour;
layout (location = 3) flat in vec4 bgColour;
layout (location = 4) flat in vec4 sizeLtRat;
layout (location = 5) flat in float gridSize;
layout (location = 6) flat in int sideData;

layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 cameraMatrix;
    vec4 cameraPos;
    vec4 data;
} sharedUboData;

void main()
{
    vec2 size = sizeLtRat.xy;
    float lineThick = sizeLtRat.z;
    float ratio = sizeLtRat.w;

    vec2 crd = mat2(1.0, -1.0, 1.0, 1.0) * vec2(localPos.x, localPos.y * ratio);
    crd = cos((crd * PI) / (size.x * gridSize / 16.0));

    float topVal = 0.0;

    float lineThickEdge = lineThick * 2.0;
    ratio =     ((-localPos.y - (0.5 - lineThickEdge)) / lineThickEdge) *   ((sideData & 0x2) != 0 ? 1.0 : 0.0);
    ratio = max(((+localPos.x - (0.5 - lineThickEdge)) / lineThickEdge) *   ((sideData & 0x4) != 0 ? 1.0 : 0.0), ratio);
    ratio = max(((+localPos.y - (0.5 - lineThickEdge)) / lineThickEdge) *   ((sideData & 0x8) != 0 ? 1.0 : 0.0), ratio);
    ratio = max(((-localPos.x - (0.5 - lineThickEdge)) / lineThickEdge) *   ((sideData & 0x10) != 0 ? 1.0 : 0.0), ratio);
    ratio = max(((min(-localPos.x, -localPos.y) - (0.5 - lineThickEdge)) / lineThickEdge) * ((sideData & 0x20) != 0 ? 1.0 : 0.0), ratio);
    ratio = max(((min(+localPos.x, -localPos.y) - (0.5 - lineThickEdge)) / lineThickEdge) * ((sideData & 0x40) != 0 ? 1.0 : 0.0), ratio);
    ratio = max(((min(+localPos.x, +localPos.y) - (0.5 - lineThickEdge)) / lineThickEdge) * ((sideData & 0x80) != 0 ? 1.0 : 0.0), ratio);
    ratio = max(((min(-localPos.x, +localPos.y) - (0.5 - lineThickEdge)) / lineThickEdge) * ((sideData & 0x100) != 0 ? 1.0 : 0.0), ratio);

    topVal = smoothstep(0.1, 1.0, ratio);

    float val = max(crd.x, crd.y);
    val = max(val, topVal);

    float sv = 1.0 - (lineThick * sharedUboData.cameraPos.z / 20.0);

    float multer = smoothstep(sv, 1.0, val);

    FragColour = lineColour * multer + bgColour * (1.0 - multer);
    //FragColour = vec4(val);
}