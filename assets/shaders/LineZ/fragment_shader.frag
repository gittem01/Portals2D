#version 460 core

layout (location = 0) out vec4 FragColor;

layout (location = 0) in vec2 localPos;
layout (location = 1) flat in vec2 size;

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
    alphaMult *= (sin(localPos.y / size.y * 40.0 - sharedUboData.data.y * 10.0) + addition) / (addition + 1.0);
    FragColor = vec4(0.0, 0.5, 1.0, alphaMult);
}