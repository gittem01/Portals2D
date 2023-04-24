#version 460 core

#define PI 3.14159265359
#define MOD3 vec3(.1031,.11369,.13787)
#define SPEED 0.03

layout (location = 0) out vec4 FragColour;

layout (location = 0) in vec2 localPos;
layout (location = 1) in vec2 realLocalPos;

vec3 hash33(vec3 p3)
{
	p3 = fract(p3 * MOD3);
    p3 += dot(p3, p3.yxz + 19.19);
    return -1.0 + 2.0 * fract(vec3((p3.x + p3.y)*p3.z, (p3.x+p3.z)*p3.y, (p3.y+p3.z)*p3.x));
}

float perlin_noise(vec3 p)
{ 
    vec3 pi = floor(p);
    vec3 pf = p - pi;
    
    vec3 w = pf * pf * (3.0 - 2.0 * pf);
    
    return 	mix(
        		mix(
                	mix(dot(pf - vec3(0, 0, 0), hash33(pi + vec3(0, 0, 0))), 
                        dot(pf - vec3(1, 0, 0), hash33(pi + vec3(1, 0, 0))),
                       	w.x),
                	mix(dot(pf - vec3(0, 0, 1), hash33(pi + vec3(0, 0, 1))), 
                        dot(pf - vec3(1, 0, 1), hash33(pi + vec3(1, 0, 1))),
                       	w.x),
                	w.z),
        		mix(
                    mix(dot(pf - vec3(0, 1, 0), hash33(pi + vec3(0, 1, 0))), 
                        dot(pf - vec3(1, 1, 0), hash33(pi + vec3(1, 1, 0))),
                       	w.x),
                   	mix(dot(pf - vec3(0, 1, 1), hash33(pi + vec3(0, 1, 1))), 
                        dot(pf - vec3(1, 1, 1), hash33(pi + vec3(1, 1, 1))),
                       	w.x),
                	w.z),
    			w.y);
}

float simplex_noise(vec3 p)
{
    const float K1 = 1.0 / 3.0;
    const float K2 = K1 * 0.5;
    
    vec3 i = floor(p + (p.x + p.y + p.z) * K1);
    vec3 d0 = p - (i - (i.x + i.y + i.z) * K2);
    
    vec3 e = step(vec3(0.0), d0 - d0.yzx);
	vec3 i1 = e * (1.0 - e.zxy);
	vec3 i2 = 1.0 - e.zxy * (1.0 - e);
    
    vec3 d1 = d0 - (i1 - 1.0 * K2);
    vec3 d2 = d0 - (i2 - 2.0 * K2);
    vec3 d3 = d0 - (1.0 - 3.0 * K2);
    
    vec4 h = max(0.6 - vec4(dot(d0, d0), dot(d1, d1), dot(d2, d2), dot(d3, d3)), 0.0);
    vec4 n = h * h * h * h * vec4(dot(d0, hash33(i)), dot(d1, hash33(i + i1)), dot(d2, hash33(i + i2)), dot(d3, hash33(i + 1.0)));
    
    return dot(vec4(31.316), n);
}

layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 cameraMatrix;
    vec4 cameraPos;
    vec4 data;
} sharedUboData;

layout (std140, set = 1, binding = 0) uniform UBO
{
    mat4 modelMatrix;
    vec4 data0; // a, b, abRatio, eThick
    vec4 data1; // lineLength, maxAngleInc, minAngleInc, noiseScale
    vec4 data2; // sizex, sizey, iTime, 0
    vec4 data3; // funcMult0, funcMult1, noiseMult, 0
    vec4 portalColour;
    vec4 edgeColour;
} ubo;

float noise(vec3 p)
{
    return pow(perlin_noise(p * 3.0), 2.0) + pow(simplex_noise(p), 1.0);
}

float getCol(vec2 uv)
{
    vec3 pos = vec3(uv * vec2(ubo.data2.x / ubo.data2.y, 1.0) * ubo.data1.w, ubo.data2.z * SPEED * 25.0);
    float f = noise(pos);
    f += 0.8;
    f = pow(f, 3.0);
    return f * ubo.data3.z;
}

vec2 getEllipsePosOnAngle(float a, float b, float angle)
{
    vec2 retVec = vec2((a * b) / sqrt(b * b + a * a * tan(angle) * tan(angle)),
                       (a * b * tan(angle)) / sqrt(b * b + a * a * tan(angle) * tan(angle)));
    
    return retVec;
}

void main()
{
    vec3 portalColor = ubo.portalColour.xyz;
    vec3 edgeColor = ubo.edgeColour.xyz;
    float eThick = ubo.data0.w;
    float lineLength = ubo.data1.x;
    
    float b = ubo.data0.y;
    float a = ubo.data0.x;
 
    float abRatio = ubo.data0.z;
 
    float func0Freq = 8.354214;
    float func0Speed = 2.146572;
    float func0MUlt = ubo.data3.x;
    
    float func1Freq = 13.87654;
    float func1Speed = 3.76543;
    float func1MUlt = ubo.data3.y;
 
    float maxAngleInc = ubo.data1.y;
    float minAngleInc = ubo.data1.z;
    
    float iTime = ubo.data2.z;

    vec2 fp = localPos;
    
    if (fp.x > lineLength * 0.5)
    {
        float noiseMult = getCol(fp / ubo.data2.xy);
        fp.x -= lineLength * 0.5;
        float angle = abs(atan(fp.y, fp.x));
        vec2 posOnEllipse = getEllipsePosOnAngle(a, b, angle);
        vec2 pos1 = getEllipsePosOnAngle(a, b, angle + 0.00001);
        float angInc = length(pos1 - posOnEllipse);
        vec2 smp = getEllipsePosOnAngle(a - eThick, b - eThick, angle);

        angInc /= maxAngleInc;

        float lengthAtAngle = length(posOnEllipse);
        float fpLengthDiff = length(fp) - lengthAtAngle;

        float fpLengthDiff2 = length(fp) - length(smp) +
            sin(angle * func0Freq * angInc * abRatio + iTime * func0Speed) * func0MUlt +
            sin(angle * func1Freq * angInc * abRatio + iTime * func1Speed) * func1MUlt;
        
        if (fpLengthDiff < 0.0 && fpLengthDiff2 > 0.0)
        {
            float ratio = min(fpLengthDiff2 / eThick, 1.0);
            FragColour = vec4(edgeColor, 1.0) * ratio + vec4(noiseMult * portalColor, 1.0) * (1.0 - ratio);
        }
        else
        {
            if (fpLengthDiff < 0.0)
            {
                FragColour = vec4(noiseMult * portalColor, 1.0);
            }
            else
            {
                float ratio = 1.0 - fpLengthDiff / 0.02;
                FragColour = vec4(edgeColor, 1.0) * ratio;
            }
        }
    }
    else if (fp.x < -lineLength * 0.5)
    {
        float noiseMult = getCol(fp / ubo.data2.xy);
        fp.x += lineLength * 0.5;
        float angle = abs(atan(fp.y, fp.x));
        vec2 posOnEllipse = getEllipsePosOnAngle(a, b, angle);
        vec2 pos1 = getEllipsePosOnAngle(a, b, angle + 0.00001);
        float angInc = length(pos1 - posOnEllipse);
        vec2 smp = getEllipsePosOnAngle(a - eThick, b - eThick, angle);

        angInc /= maxAngleInc;

        float lengthAtAngle = length(posOnEllipse);
        float fpLengthDiff = length(fp) - lengthAtAngle;

        float fpLengthDiff2 = length(fp) - length(smp) +
            sin(angle * func0Freq * angInc * abRatio + iTime * func0Speed) * func0MUlt +
            sin(angle * func1Freq * angInc * abRatio + iTime * func1Speed) * func1MUlt;

        if (fpLengthDiff < 0.0 && fpLengthDiff2 > 0.0)
        {
            float ratio = min(fpLengthDiff2 / eThick, 1.0);
            FragColour = vec4(edgeColor, 1.0) * ratio + vec4(noiseMult * portalColor, 1.0) * (1.0 - ratio);
        }
        else
        {
            if (fpLengthDiff < 0.0)
            {
                FragColour = vec4(noiseMult * portalColor, 1.0);
            }
            else
            {
                float ratio = 1.0 - fpLengthDiff / 0.02;
                FragColour = vec4(edgeColor, 1.0) * ratio;
            }
        }
    }
    else if (fp.x >= -lineLength * 0.5 && fp.x <= lineLength * 0.5)
    {
        if (fp.y < b && fp.y > -b)
        {
            float noiseMult = getCol(fp / ubo.data2.xy);
            float ratio = abs(fp.x) / lineLength;
            ratio += 1.0;
            ratio *= 1.0 / 3.0;
            float fpLengthDiff2 = fp.y + eThick - b +
                sin(ratio * func0Freq * (minAngleInc / maxAngleInc) * PI * abRatio + iTime * func0Speed) * func0MUlt +
                sin(ratio * func1Freq * (minAngleInc / maxAngleInc) * PI * abRatio + iTime * func1Speed) * func1MUlt;
        
            float fpLengthDiff3 = -fp.y + eThick - b +
                sin(ratio * func0Freq * (minAngleInc / maxAngleInc) * PI * abRatio + iTime * func0Speed) * func0MUlt +
                sin(ratio * func1Freq * (minAngleInc / maxAngleInc) * PI * abRatio + iTime * func1Speed) * func1MUlt;
        
            if (fpLengthDiff2 > 0.0)
            {
                float ratio2 = min(fpLengthDiff2 / eThick, 1.0);
                FragColour = vec4(edgeColor, 1.0) * ratio2 + vec4(noiseMult * portalColor, 1.0) * (1.0 - ratio2);
            }
            else if (fpLengthDiff3 > 0.0)
            {
                float ratio2 = min(fpLengthDiff3 / eThick, 1.0);
                FragColour = vec4(edgeColor, 1.0) * ratio2 + vec4(noiseMult * portalColor, 1.0) * (1.0 - ratio2);
            }
            else
            {
                FragColour = vec4(noiseMult * portalColor, 1.0);
            }
        }
        else
        {
            float ratio = 1.0 - (abs(fp.y) - b) / 0.02;
            FragColour = vec4(edgeColor, 1.0) * ratio;
        }
    }
    else
    {
        FragColour = vec4(0.0);
    }
}