#version 460 core

layout (location = 0) out vec4 FragColor;

layout (location = 0) in vec2 localPos;

layout (push_constant) uniform PC
{
    vec4 posSize;
    ivec4 data; // mask, unused, unused, unused
} pc;

float sdBox(in vec2 p, in vec2 b)
{
    vec2 d = abs(p) - b;
    return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0);
}

float sdLine(in vec2 p, float x)
{
    return p.x - x;
}

float sdLineH(in float yPos, float sizeMult)
{
    return abs(mod(yPos - 1.0 * sizeMult, 2.0 * sizeMult) - 1.0 * sizeMult);
}

float sdCircle(in vec2 p, in float r) 
{
    return length(p) - r;
}

float udSegment(in vec2 p, in vec2 a, in vec2 b)
{
    vec2 ba = b-a;
    vec2 pa = p-a;
    float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    
    return length(pa - h * ba);
}

float arrow(vec2 p, vec2 arrowPoint, float sizeMult)
{
    float s1 = udSegment(p, arrowPoint, arrowPoint + vec2(-0.2, +0.2) * sizeMult);
    float s2 = udSegment(p, arrowPoint, arrowPoint + vec2(-0.2, -0.2) * sizeMult);
    
    return min(s1, s2);
}

void main()
{
    float sizeMult = 4.0;
    vec2 boxSize = pc.posSize.zw;
    vec2 p = localPos * 2.0;
    p -= boxSize;
    
	float d = sdBox(p + boxSize, boxSize);

    const float smoother = 0.15 * sizeMult;

    float lMult = (sign(-d) + 1.0) * 0.5;

    vec4 col = (d > 0.0) ? vec4(0.0, 0.0, 0.0, 0.0) : vec4(1.0, 1.0, 1.0, 1.0);
    
    float lsd;

    if (pc.data.y > 0)
    {
        lsd = sdLine(p, -boxSize.x * 2.0 + 2.0 * sizeMult);
        col = mix(col, vec4(0.35, 0.35, 0.35, 1.0), (1.0 - smoothstep(0.0, smoother * 0.4, lsd)) * lMult);
    }
    
    col = mix(col, vec4(0.0, 0.0, 0.0, 1.0), 1.0 - smoothstep(0.0, smoother, abs(d)));
    
    lsd = sdLineH(p.y, sizeMult);
    col = mix(col, vec4(0.0, 0.0, 0.0, 1.0), (1.0 - smoothstep(0.0, smoother * 0.6, lsd)) * lMult);
    
    int index = int((-p.y) / (sizeMult * 2.0));
    
    int mask = pc.data.x;
    if ((1 << index & mask >> 16) > 0)
    {
        lsd = arrow(p, vec2(-0.5 * sizeMult, -float(index) * 2.0 * sizeMult - sizeMult), sizeMult);
        col = mix(col, vec4(0.0, 0.0, 0.0, 1.0), (1.0 - smoothstep(0.0, smoother * 1.5, lsd)) * lMult);
    }
    
    if ((mask >> 24 & 0xF) == index)
    {
        col.xyz -= vec3(col.x) * lMult * 0.4;
    }
    
    if ((1 << index & mask) > 0)
    {
        lsd = sdCircle(p + vec2(boxSize.x * 2.0 - sizeMult, float(index) * 2.0 * sizeMult + sizeMult), 0.6 * sizeMult);
        col = mix(col, vec4(0.02, 0.05, 0.02, 1.0), (1.0 - smoothstep(0.0, smoother * 0.6, lsd)) * lMult);
    
        if ((1 << index & mask >> 8) > 0)
        {
            lsd = sdCircle(p + vec2(boxSize.x * 2.0 - sizeMult, float(index) * 2.0 * sizeMult + sizeMult), 0.4 * sizeMult);
            col = mix(col, vec4(0.05, 1.0, 0.2, 1.0), (1.0 - smoothstep(0.0, smoother * 0.6, lsd)) * lMult);
        }
    }    
    
    FragColor = col;
}