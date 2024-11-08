#version 460 core

layout (location = 0) in vec2 inPos;
layout (location = 1) in vec2 i_texCoord;

layout (location = 0) out vec2 o_texCoord;
layout (location = 1) out vec2 worldPosition;
layout (location = 2) out vec2 localPosition;

layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 orthoMatrix;
    vec4 cameraPos;
    vec4 data; // zoom, unused, unused, unused
} sharedUboData;

layout (push_constant) uniform PC
{
    vec4 posRot; // posx, posy, rot, mirrorStatus(<0 for mirrored)
	vec4 movScale; // xMov, yMov, xScale, yScale (movement and scaling before rotation)
	vec4 portals[4];
	vec4 data; // numPortals, destroyTime, gelColorType (<-5 for no gel), unused
} pc;

void main() 
{
	o_texCoord = i_texCoord;
	
	mat2 rotMat =	mat2(
							+cos(pc.posRot.z),	sin(pc.posRot.z),
							-sin(pc.posRot.z),	cos(pc.posRot.z)
						);
	vec2 pos = inPos;
	if (pc.posRot.w < 0)
	{
		pos.x *= -1.0;
	}

	localPosition = pos * pc.movScale.zw;

	pos = rotMat * (localPosition + pc.movScale.xy);

	vec4 wp = vec4(pos + pc.posRot.xy, 0.0, 1.0);
	worldPosition = wp.xy;

	gl_Position = sharedUboData.orthoMatrix * wp;
}