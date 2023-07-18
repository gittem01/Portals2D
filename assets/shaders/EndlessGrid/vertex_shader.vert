#version 460 core

#define PI 3.14159265359

layout (location = 0) in vec2 vertex;

layout (location = 0) out vec2 localPos;
layout (location = 1) flat out vec2 size;
layout (location = 2) out vec2 worldPos;

layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 cameraMatrix;
    vec4 cameraPos; // pos.xy, size.xy
    vec4 data;
} sharedUboData;


void main()
{
    int index = gl_InstanceIndex;

    float zoom = sharedUboData.data.x;
    float zoomSqr = zoom * zoom;
    vec2 camPos = vec2(sharedUboData.cameraPos.x, -sharedUboData.cameraPos.y);
    vec2 pos;
    const float thick = 0.05 / zoomSqr;
    if (index % 2 == 0)
    {
        pos = vec2(float(index / 2), camPos.y);
        size = vec2(thick, 100.0f);
    }
    else
    {
        pos = vec2(camPos.x, float(index / 2));
        size = vec2(100.0f, thick);
    }

    pos -= camPos;

    const float xCopySize = 50.0;
	const float yCopySize = 50.0;

    pos.x = mod(pos.x + xCopySize, xCopySize * 2.0) - xCopySize;
    pos.y = mod(pos.y + yCopySize, yCopySize * 2.0) - yCopySize;

	pos += camPos;

    localPos = vertex;
    worldPos = vertex * size + pos;
    vec4 outPos = sharedUboData.cameraMatrix * vec4(worldPos, 0.0, 1.0);

    gl_Position = outPos;
}