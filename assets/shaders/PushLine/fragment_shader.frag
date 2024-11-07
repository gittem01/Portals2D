#version 460 core

layout (location = 0) out vec4 color;

layout (location = 0) in vec2 localPos;
layout (location = 1) flat in vec2 size;
layout (location = 2) flat in int instanceIndex;

layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 orthoMatrix;
    vec4 cameraPos;
    vec4 data; // zoom, unused, unused, unused
} sharedUboData;

layout (push_constant) uniform PC
{
    vec4 positions;
    vec4 colour;
    vec4 data; // unused, lineThickness, outerSmoothThicknessRatio, unused
} pc;

void main()
{
    float lp2x = localPos.x + size.x * 0.5 + size.y;
    if (mod(lp2x, pc.data.x) < pc.data.w)
    {
        color = vec4(0.0f, 0.0f, 0.0f, 0.0f);
        return;
    }

    float zoom = sharedUboData.data.x;
    float perctDist = 1.0f;
    vec2 p1 = vec2(size.x - size.y, 0.0f);
    vec2 p2 = vec2(-size.x + size.y, 0.0f);
    if (localPos.x < p2.x && length(localPos - p2) > size.y || localPos.x > p1.x && length(localPos - p1) > size.y && false) // may be removed?
    {
        color = vec4(0.0f, 0.0f, 0.0f, 0.0f);
        return;
    }
    else
    {
        if (localPos.x < p2.x)
        {
            perctDist = length(localPos - p2) / size.y;
        }
        else if (localPos.x > p1.x)
        {
            perctDist = length(localPos - p1) / size.y;
        }
        else
        {
            perctDist = abs(localPos.y / size.y);
        }
    }

    color = vec4(pc.colour.xyz, pc.colour.w * (1.0f - perctDist) * (1.0f / (1.0f - pc.data.z)));
}