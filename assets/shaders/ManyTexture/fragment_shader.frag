#version 460 core

layout (location = 0) out vec4 FragColour;

layout (location = 0) in vec4 localPos;
layout (location = 1) in vec4 worldPos;
layout (location = 2) in vec2 tc;


layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 cameraMatrix;
    vec4 cameraPos;
    vec4 data;
} sharedUboData;

layout(set = 2, binding = 0) uniform sampler2D tex;

void main()
{   
    vec4 txClr = texture(tex, tc);

    FragColour = txClr;
}