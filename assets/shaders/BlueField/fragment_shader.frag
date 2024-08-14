#version 460 core

#define PI 3.14159265359

layout (location = 0) out vec4 FragColour;

layout (location = 0) in vec2 localPos;
layout (location = 1) in vec2 worldPos;
layout (location = 2) flat in vec2 size;
layout (location = 3) flat in vec4 colour;
layout (location = 4) flat in vec4 colour2;
layout (location = 5) flat in vec4 data;
layout (location = 6) flat in vec4 shootData;

layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 cameraMatrix;
    vec4 cameraPos;
    vec4 data;
} sharedUboData;

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
	        vec2 r = g - f + (0.5 + 0.5 * sin(sharedUboData.data.y * 2.0 + 6.0 * o));
		    float d = dot(r, r);
            if(d < m)
            {
                m = d;
            }
        }
    }

    return sqrt(m);
}

void main()
{
    float c = voronoi(localPos * 10.0 + worldPos);
    float c2 = voronoi(localPos * 3.0 + worldPos);
    float v = 1.0 - pow(c + c2, 3.0);
    
    float sideMult = smoothstep(0.0, 0.5, 2.0 * (size.y * 0.5 - abs(localPos.y)) / size.y);

    v *= sideMult;

    const float smoothSize = min(size.x * 0.1, 5.0);
    float timeDiff = (sharedUboData.data.y - data.w) * min(200.0, size.x * 5.0);
    float splitX = (-smoothSize + timeDiff) * data.z + (size.x * 0.5 + smoothSize - timeDiff) * (1.0 - data.z);

    float fullMult = min(max(splitX - (size.x * 0.5 - abs(localPos.x)), 0.0), smoothSize) / smoothSize;

    float currentRadius = max((sharedUboData.data.y - shootData.z), 0.01) * 10.0;
    vec2 worldDiff = worldPos - shootData.xy;
    float wl = length(worldDiff);

    float add = max((currentRadius - abs(currentRadius - wl)) * 4.0, 0.0) / pow(currentRadius, 3.0);
    c2 = add;
    
    FragColour = (colour * v + pow(c2, 2.0) * colour2 * sideMult * 0.5) * fullMult;
}