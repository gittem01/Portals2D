#version 460 core

#define PI 3.14159265359

layout (location = 0) out vec4 FragColour;

layout (location = 0) in vec2 localPos;
layout (location = 1) in vec2 worldPos;
layout (location = 2) flat in vec2 size;
layout (location = 3) flat in vec4 inColour;
layout (location = 4) flat in vec4 outColour;
layout (location = 5) flat in vec4 outData;
layout (location = 6) flat in int inInstanceIndex;

layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 orthoMatrix;
    vec4 cameraPos;
    vec4 data; // zoom, unused, unused, unused
} sharedUboData;

// { 2d cell id, distance to border )
vec3 hexagon(vec2 p) 
{
	vec2 q = vec2(p.x * 2.0 * 0.5773503, p.y + p.x * 0.5773503);
	
	vec2 pi = floor(q);
	vec2 pf = fract(q);

	float v = mod(pi.x + pi.y, 3.0);

	float ca = step(1.0, v);
	float cb = step(2.0, v);
	vec2 ma = step(pf.xy, pf.yx);
	
	float e = dot(ma, 1.0 - pf.yx + ca*(pf.x + pf.y - 1.0) + cb*(pf.yx - 2.0 * pf.xy));
	
	return vec3(pi + ca - cb*ma, e);
}

float hash(ivec2 p)
{
    int n = p.x*3 + p.y*113;

	n = (n << 13) ^ n;
    n = n * (n * n * 15731 + 789221) + 1376312589;
    return -1.0 + 2.0 * float(n & 0x0fffffff) / float(0x0fffffff);
}

void main()
{
    float sv = outData.x;
    float sider = outData.y;

    vec3 h = hexagon(4.0 * localPos);
    float hashMult = 0.75 + 0.25 * hash(ivec2(h.xy + float(inInstanceIndex) * 100.0));

    float m = smoothstep(outData.x - 0.1, 0.1 + outData.x, h.z) * (inColour.w > 0.0 ? hashMult : 1.0);

	vec4 extraColor = m * inColour + (1.0 - m) * outColour;
    
    float xDist = size.x * 0.5 - abs(localPos.x);
    float yDist = size.y * 0.5 - abs(localPos.y);
    float dist = min(xDist, yDist);
    
    float n = 0.0;

    if (sv > 0.0 && false)
    {
        n += abs(mod(abs(localPos.x), sv) - sv * 0.5);
        n += abs(mod(abs(localPos.y), sv) - sv * 0.5);
    }

    if (dist < sider * 0.5)
    {
        float v = smoothstep(0.0, 0.4, dist / (sider * 0.5));
        FragColour = outColour * v + (1.0 - v) * vec4(0.0, 0.0, 0.0, 0.0) + n;
    }
    else if (dist < sider)
    {
        float v = smoothstep(0.0, 1.0, 1.0 - (dist - sider * 0.5) / (sider * 0.5));
        FragColour = outColour * v + (1.0 - v) * (n + extraColor);
    }
    else
    {
        FragColour = n + extraColor;
    }
}