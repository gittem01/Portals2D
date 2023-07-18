#version 460 core

layout (location = 0) out vec4 FragColor;

layout (location = 0) in vec2 texCoord;

layout(set = 1, binding = 0) uniform sampler2D tex;

void main()
{
	vec4 clr = texture(tex, texCoord);
	FragColor = vec4(1.0, 1.0, 1.0, clr.x);
}