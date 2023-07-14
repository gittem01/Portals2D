#version 460 core

layout (location = 0) in vec2 inPos;
layout (location = 1) in vec2 i_texCoord;

layout (location = 0) out vec2 o_texCoord;
layout (location = 1) out vec2 worldPosition;

layout (set = 0, binding = 0) uniform SharedUboData
{
	mat4 orthoMatrix;
} sharedUboData;

layout (set = 2, binding = 0) uniform UBO
{
	mat4 modelMatrix;
	int isMirrored;
} ubo;

void main() 
{
	o_texCoord = i_texCoord;
	vec2 pos = inPos.xy;
	if (ubo.isMirrored == 1)
	{
		pos.x *= -1.0;;
	}
	vec4 wp = ubo.modelMatrix * vec4(pos, 0.0, 1.0f);
	worldPosition = wp.xy;

	gl_Position = sharedUboData.orthoMatrix * wp;
}