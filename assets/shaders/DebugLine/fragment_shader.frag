#version 460 core

layout (location = 0) out vec4 color;

layout (location = 0) in vec2 localPos;
layout (location = 1) in vec2 size;

layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 orthoMatrix;
    vec4 data; // zoom, unused, unused, unused
} sharedUboData;

layout (push_constant) uniform UBO
{
    vec4 positions;
    vec4 colour;
    vec4 data; // unused, lineThickness, outerSmoothThicknessRatio, unused
} ubo;

void main()
{
    float zoom = sharedUboData.data.x;
    float perctDist = 1.0f;
    vec2 p1 = vec2(size.x - size.y, 0.0f);
    vec2 p2 = vec2(-size.x + size.y, 0.0f);
    if (localPos.x < p2.x && length(localPos - p2) > size.y || localPos.x > p1.x && length(localPos - p1) > size.y){
        color = vec4(0, 0, 0.0f, 0.0f);
        return;
    }
    else
    {
        if (localPos.x < p2.x)
        {
            perctDist = length(localPos - p2) / (size.y / (zoom * zoom));
        }
        else if (localPos.x > p1.x)
        {
            perctDist = length(localPos  - p1) / (size.y / (zoom * zoom));
        }
        else
        {
            perctDist = abs(localPos.y / (size.y / (zoom * zoom)));
        }
    }

    perctDist /= zoom * zoom;
    if (perctDist > ubo.data.z)
    {
        color = vec4(ubo.colour.xyz, (1.0f - perctDist) * (1.0f / (1.0f - ubo.data.z)));
    }
    else
    {
        color = ubo.colour;
    }
}