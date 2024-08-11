#version 460 core

layout (location = 0) out vec4 outFragColor;

layout (location = 0) in vec2 texCoord;
layout (location = 1) in vec2 worldPosition;

layout(set = 1, binding = 0) uniform sampler2D tex;

layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 cameraMatrix;
    vec4 cameraPos;
    vec4 data;
} sharedUboData;

layout (push_constant) uniform PC
{
    vec4 posRot; // posx, posy, rot, mirrorStatus(<0 for mirrored)
	vec4 movScale; // xMov, yMov, xScale, yScale (movement and scaling before rotation)
	vec4 portals[4];
	vec4 data; // numPortals, destroyTime, unused, unused
} pc;

float hash(vec2 p)
{
    p  = 50.0 * fract(p * 0.3183099 + vec2(0.71, 0.113));
    return -1.0 + 2.0 * fract(p.x * p.y * (p.x + p.y));
}

float noise( in vec2 p )
{
    ivec2 i = ivec2(floor( p ));
    vec2 f = fract( p );
	
    vec2 u = f*f*(3.0-2.0*f);

    return mix( mix( hash( i + ivec2(0,0) ), 
                     hash( i + ivec2(1,0) ), u.x),
                mix( hash( i + ivec2(0,1) ), 
                     hash( i + ivec2(1,1) ), u.x), u.y);
}

float lineRel(vec2 a, vec2 b, vec2 c)
{
     return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

void main()
{
	for (int i = 0; i < int(round(pc.data.x)); i++)
	{
		if (lineRel(pc.portals[i].xy, pc.portals[i].zw, worldPosition) > 0)
		{
			outFragColor = vec4(0.0f, 0.0f, 0.0f, 0.0f);
			return;
		}
	}
	
	float f = 1.0f;
    float blackAmount = 0.0;
	if (pc.data.y > -10.0f)
	{
        float iTime = sharedUboData.data.y - pc.data.y;
        vec2 uv = worldPosition;
        uv *= 4.0;
		mat2 m = mat2(1.6,  1.2, -1.2,  1.6);
        f  = 0.5000 * noise(uv + iTime); uv = m * uv;
        f += 0.2500 * noise(uv + iTime * 1.5); uv = m * uv;
        f += 0.1250 * noise(uv - iTime * 4.0); uv = m * uv;
        f += 0.0625 * noise(uv + iTime * 3.0); uv = m * uv;

        f += 1.0;
        f -= iTime * 2;
        f *= 4.0;
        f = min(f, 1.0);

        blackAmount = pow(iTime, 0.5);
	}
    
    vec4 clr = texture(tex, texCoord);
    clr.rgb *= 1.0 - blackAmount;

    float yellowAmount = 0.0;
    if (f > 0.01 && f < 0.99)
	{
		yellowAmount = pow(1.0 - abs(0.5 - f) * 2.0, 2.0);
	}

	outFragColor = clr * f * (1.0 - yellowAmount) + vec4(yellowAmount, yellowAmount, 0.0, yellowAmount * clr.w);
}