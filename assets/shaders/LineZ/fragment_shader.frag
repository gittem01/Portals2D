#version 460 core

layout (location = 0) out vec4 FragColor;

layout (location = 0) in vec2 localPos;
layout (location = 1) flat in vec2 size;
layout (location = 2) flat in vec4 colour;

layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 cameraMatrix;
    vec4 cameraPos;
    vec4 data;
} sharedUboData;

void main()
{
    float alphaMult = smoothstep(0.0, 0.8, 1.0 - abs(localPos.x / size.x) * 2.0) * smoothstep(0.0, 0.1, (size.y * 0.5 - abs(localPos.y)));
    const float addition = 1.5;
    alphaMult *= (sin(localPos.y * 10.0 - sharedUboData.data.y * 3.0) + addition) / (addition + 1.0);

    float posPos = localPos.y + size.y * 0.5 - sharedUboData.data.y;
    float modded = mod(posPos, size.x) - size.x * 0.5;
    float dist = (size.x * 0.5 - length(vec2(modded, localPos.x))) / (size.x * 0.5);
    dist = smoothstep(0.0, 0.5, dist);

    FragColor = vec4(colour.rgb, alphaMult * colour.a) * dist;
}