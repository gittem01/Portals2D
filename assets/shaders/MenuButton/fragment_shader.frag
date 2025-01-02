#version 460 core

layout (location = 0) out vec4 FragColour;

layout (location = 0) in vec2 localPos;
layout (location = 1) in vec2 worldPos;

layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 orthoMatrix;
    vec4 cameraPos;
    vec4 data; // zoom, unused, unused, unused
} sharedUboData;

layout (push_constant) uniform PC
{
    vec4 posSize;
    vec4 colour;
    vec4 data; // extraFill, smoothFill, noise alpha, noise scaler
} pc;


vec2 hash(vec2 p)
{
    p = vec2(dot(p, vec2(633.7, 472.7)), dot(p, vec2(147.9, 742.1)));
    return fract(sin(p) * 16.3561);
}

float voronoi(vec2 x)
{
    vec2 n = floor(x);
    vec2 f = fract(x);

	float m = 8.0;
    for(int j = -1; j <= 1; j++)
    {
        for(int i = -1; i <= 1; i++)
        {
            vec2 g = vec2(float(i), float(j));
            vec2 o = hash(n + g);
	        vec2 r = g - f + (0.5 + 0.5 * sin(sharedUboData.data.y * 0.5 + 6.0 * o));
		    float d = dot(r, r);
            if(d < m)
            {
                m = d;
            }
        }
    }

    return sqrt(m);
}

float boxSDF(vec2 p, vec2 b)
{
    vec2 d = abs(p) - b;
    return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0);
}

void main()
{
    vec2 size = pc.posSize.zw;
    vec4 colour = pc.colour;
    float extra = pc.data.x;
    float smoother = pc.data.y;

    vec2 p = localPos;
    float d = 2.0 * boxSDF(p, size * 0.5) - extra;

    if (pc.data.z > 0.0001)
    {
        float noiseVal = pc.data.z - voronoi(worldPos * pc.data.w) * 0.5;
        colour.w *= noiseVal;
    }
    
    if (d < 0.0)
    {
        FragColour = colour;
    }
    else
    {
        d /= smoother - extra;
        colour.w *= (1.0 - d);
        FragColour = colour;
    }
}