#version 460 core

layout (location = 0) out vec4 FragColour;

layout (location = 0) in vec2 localPos;
layout (location = 1) in vec2 worldPos;
layout (location = 2) flat in vec2 size;

layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 cameraMatrix;
    vec4 cameraPos;
    vec4 data;
} sharedUboData;

layout (push_constant) uniform PC
{
    vec4 pointts;
    vec4 colour;
    vec4 data; // thickness, none, none, none
} pc;

float hash(ivec2 p)
{
    int n = p.x*3 + p.y*113;

	n = (n << 13) ^ n;
    n = n * (n * n * 15731 + 789221) + 1376312589;
    return -1.0 + 2.0 * float(n & 0x0fffffff) / float(0x0fffffff);
}

float noise(vec2 p)
{
    ivec2 i = ivec2(floor(p));
    vec2 f = fract(p);

    vec2 u = f * f * (3.0 - 2.0 * f);

    return mix(mix(hash(i + ivec2(0, 0)), 
                   hash(i + ivec2(1, 0)), u.x),
               mix(hash(i + ivec2(0, 1)), 
                   hash(i + ivec2(1, 1)), u.x), u.y);
}

float getNoise()
{
    mat2 m = mat2( 1.6,  1.2, -1.2,  1.6 );
    float f = 0.0;
    float sv = 0.5;
    int mx = 5;
    vec2 uv = worldPos + vec2(sharedUboData.data.y * 0.5, sharedUboData.data.y * 0.363);
    uv *= 4.0;
    for (int i = 0; i < mx; i++)
    {
        f += sv * noise(uv);
        uv = m * uv;
        sv /= 2.0;
    }

    return 0.8 + 0.2 * f;
}

void main()
{
    vec4 revClr = (vec4(1.0) - pc.colour) * 5.0;
    
    vec2 uv = localPos;
    float v = pc.data.x * 0.5;
    if (abs(uv.y) < v)
    {
        if (abs(uv.x) < size.x - size.y)
        {
            float val10 = 1.0 - abs(uv.y) * 1.0 / v;
            val10 = smoothstep(0.0, pc.data.y, val10) * pc.data.z;
            float f = getNoise();

            FragColour = (vec4(1.0 + f) - revClr * (1.0 - val10)) * val10;
        }
        else if (abs(uv.x) >= size.x - size.y)
        {
            float val10 = 1.0 - length(abs(uv) - vec2(size.x - size.y, 0.0)) / v;
            val10 = smoothstep(0.0, pc.data.y, val10) * pc.data.z;

            float f = getNoise();

            FragColour = (vec4(1.0 + f) - revClr * (1.0 - val10)) * val10;
        }
    }
    else
    {
        FragColour = vec4(0.0);
    }
}