#version 460 core

layout (location = 0) out vec4 outFragColor;

layout (location = 0) in vec2 texCoord;
layout (location = 1) in vec2 worldPosition;

layout(set = 1, binding = 0) uniform sampler2D tex;

layout (set = 0, binding = 2) uniform Ubo
{
	int numPortals;
} ubo;

layout (set = 0, binding = 3) uniform PortalData
{
	vec4 portals[256];
} portalData;

float lineRel(vec2 a, vec2 b, vec2 c)
{
     return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

void main()
{
	for (int i = 0; i < ubo.numPortals; i++)
	{
		if (lineRel(portalData.portals[i].xy, portalData.portals[i].zw, worldPosition) > 0){
			outFragColor = vec4(0.0f, 0.0f, 0.0f, 0.0f);
			return;
		}
	}

	outFragColor = texture(tex, texCoord);
}