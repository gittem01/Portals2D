#version 460 core

layout (location = 0) out vec4 FragColour;

layout (location = 0) in vec4 normal;
layout (location = 1) in vec4 worldPos;
layout (location = 2) in vec2 tc;

layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 cameraMatrix;
    vec4 cameraPos;
    vec4 data;
} sharedUboData;

layout (std140, set = 1, binding = 1) uniform UBO_FIX
{
    vec4 portals[4];
    vec4 data; // numPortals, unused, unused, unused
} ubo_FIX;

layout(set = 2, binding = 0) uniform sampler2D tex;

float lineRel(vec2 a, vec2 b, vec2 c)
{
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

void main()
{
    for (int i = 0; i < int(round(ubo_FIX.data.x)); i++)
    {
        if (lineRel(ubo_FIX.portals[i].xy, ubo_FIX.portals[i].zw, worldPos.xy) > 0)
        {
            FragColour = vec4(0.0f, 0.0f, 0.0f, 0.0f);
            return;
        }
    }

    vec4 lightDir = normalize(worldPos - sharedUboData.cameraPos);
    lightDir = normalize(vec4(-1.0, -1.0, -0.3, 0.0));

    float camDot = max(dot(lightDir, -normal), 0.0);

    const float ratio = 0.7;

    vec3 texColor = texture(tex, tc).xyz;

    FragColour = vec4(texColor * camDot * ratio + texColor * (1.0 - ratio), 1.0);

    return;
}