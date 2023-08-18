#version 460 core

#define PI 3.14159265359


layout (location = 0) out vec4 FragColour;

layout (location = 0) in vec2 localPos;
layout (location = 1) in vec2 worldPos;
layout (location = 2) flat in vec4 lineColour;
layout (location = 3) flat in vec4 bgColour;
layout (location = 4) flat in vec4 sizeLtRat;
layout (location = 5) flat in float gridSize;

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

    float val = max(crd.x, crd.y);

    float sv = 1.0 - (lineThick * sharedUboData.cameraPos.z / 20.0);

    float multer = smoothstep(sv, 1.0, val);

    FragColour = lineColour * multer + bgColour * (1.0 - multer);
}