#version 460 core

#define PI 3.141592653589793

vec4 colors[3] = 
{
    vec4(0.0, 0.4, 1.0, 1.0),
    vec4(1.0, 0.4, 0.0, 1.0),
    vec4(1.0, 1.0, 1.0, 1.0),
};

layout (location = 0) out vec4 outFragColor;

layout (location = 0) in vec2 localPosition;
layout (location = 1) flat in ivec4 inData;
layout (location = 2) flat in float angle;
layout (location = 3) in vec2 wp;

layout(set = 1, binding = 0) uniform sampler2D tex;

layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 cameraMatrix;
    vec4 cameraPos;
    vec4 data;
} sharedUboData;

struct perGelLineData
{
    vec4 positions;
    ivec4 data; // size, gelDataIndex, prevIndex, nextIndex
};

struct perGelTickData
{
    vec4 data; // colorIndex, gelHeight, colorIndex, gelHeight
};

layout (std140, set = 1, binding = 0) readonly buffer UBO
{
    perGelLineData sData[];
} ubo;

layout (std140, set = 1, binding = 1) readonly buffer UBO2
{
    perGelTickData sData[];
} ubo2;

vec2 rotateAroundOrigin(vec2 point, float angle)
{
    float s = sin(angle);
    float c = cos(angle);

    vec2 p;

    p.x = point.x * c - point.y * s;
    p.y = point.y * c + point.x * s;

    return p;
}

float getFinalColor(vec3 normal, vec3 lightDir)
{
    float NdotL = max(dot(normalize(normal), normalize(lightDir)) + 1.0, 0.0);
    return NdotL;
}

float getSpecularComponent(vec3 normal, vec3 lightDir, vec3 viewDir, float shininess) {
    vec3 R = reflect(-lightDir, normal);

    float RdotV = max(dot(R, viewDir), 0.0);
    return pow(RdotV, shininess);
}

float rand(vec2 seed)
{
    return fract(sin(dot(seed, vec2(2.422001, 5.3131))) * 5.71);
}

float opSmoothUnion(float d1, float d2, float k)
{
    float h = max(k-abs(d1-d2),0.0);
    return min(d1, d2) - h*h*0.25/k;
}

float vecMax(vec3 v)
{
    return max(max(v.x, v.y), v.z);
}

const float perTileGel = 10.0;

void calculate(vec2 pos, float lastVal, vec3 finalClr, int indOffset, int maxSize, out vec3 oNorm, out float oVal, out vec3 oColor)
{
    vec3 norm = vec3(0.0);
    for (int i = -1; i < 2; i++)
    {
        float checkX = int(pos.x * perTileGel) + i;
        int ind = int(checkX);
        if (ind >= maxSize || ind < 0) continue;
        ind += indOffset;
        float randVal = rand(vec2(ind * 0.18811938, ind * 0.10711453));
        randVal = sin(sharedUboData.data.y * randVal);
        vec4 dt = ubo2.sData[ind / 2].data;
        int remInd = (ind % 2) * 2;
        float val = dt[remInd + 1] - 0.25;
        val *= 0.8 + randVal * 0.1;
        if (val < 0.0) val = -0.25;
        vec3 clr = colors[int(round(dt[remInd]))].xyz;
        vec2 diff = vec2(pos.x * perTileGel - 0.5, pos.y * 0.4 * perTileGel) - vec2(checkX, min(val, pos.y) * 0.4 * perTileGel);
        float revDiff = pow(max(1.0 - length(diff), 0.0), 2.0);
        norm += normalize(vec3(-diff, revDiff * 3.0)) * revDiff;
        finalClr += clr * pow(revDiff, 0.5);
        lastVal = opSmoothUnion(lastVal, pow(length(diff), 3.0), 1.0);
    }

    oNorm = norm;
    oVal = lastVal;
    oColor = finalClr;
}

void main()
{
    vec3 colors[3] = {
        vec3(0.0, 0.5, 1.0),
        vec3(1.0, 0.5, 0.0),
        vec3(1.0, 1.0, 1.0),
    };

    float iTime = sharedUboData.data.y;
    float xSize = inData.x * 0.1;

	vec2 pos = localPosition;
    vec3 norm = vec3(0.0, 0.0, 0.0);
    float lastVal = 1.0;
    vec3 finalClr = vec3(0.0, 0.0, 0.0);
    
    calculate(pos, lastVal, finalClr, inData.y, inData.x, norm, lastVal, finalClr);

    norm.xy = rotateAroundOrigin(norm.xy, -angle);

    if (pos.x < 0.5 && inData.z >= 0)
    {
        float tmp1 = pos.x;
        float tmp2 = pos.y;
        pos.y = pos.x * 2.5;
        pos.x = 1.0 - tmp2 * 0.4;

        vec3 n;
        calculate(pos, lastVal, finalClr, inData.z, 10, n, lastVal, finalClr);
        norm = norm + vec3(rotateAroundOrigin(n.xy, -angle - PI * 0.5), n.z);

        pos.x = tmp1;
        pos.y = tmp2;
    }

    if (pos.x > xSize - 0.5 && inData.w >= 0)
    {
        float tmp1 = pos.x;
        float tmp2 = pos.y;
        pos.y = (xSize - pos.x) * 2.5;
        pos.x = tmp2 * 0.4;

        vec3 n;
        calculate(pos, lastVal, finalClr, inData.w, 10, n, lastVal, finalClr);
        norm = norm + vec3(rotateAroundOrigin(n.xy, -angle + PI * 0.5), n.z);

        pos.x = tmp1;
        pos.y = tmp2;
    }
    
    vec3 gelClr = 2.5 * finalClr / vecMax(finalClr);

    const float vx = 0.1;
    const float vy = vx * 2.5;
    float multy = (max(vy - pos.y, 0.0)) / vy;

    float xVal = (pos.x > 0.5) ? xSize - pos.x : pos.x;
    float multx = (max(vx - xVal, 0.0)) / vx;

    norm = normalize(norm);

    if (lastVal < 0.5)
    {
        vec3 lightDir = normalize(vec3(-1.0, -1.0, 1.0));
        vec3 camPos = sharedUboData.cameraPos.xyz;
        vec3 viewDir = vec3(wp, 0.0) - vec3(camPos.xy * vec2(1.0, -1.0), -1.0);
        viewDir.xy /= camPos.z;
        viewDir = normalize(viewDir);
        viewDir.y *= -1.0;

        float outClr = 0.0;
        const float powVal = 50.0;
        const float multVal = 10.0;

        lightDir = normalize(vec3(1.0, 0.0, 1.0));
        outClr += getSpecularComponent(norm, lightDir, viewDir, powVal) * multVal;
        outClr += getFinalColor(norm, lightDir);
        lightDir = normalize(vec3(-1.0, 0.0, 1.0));
        outClr += getSpecularComponent(norm, lightDir, viewDir, powVal) * multVal;
        outClr += getFinalColor(norm, lightDir);
        lightDir = normalize(vec3(0.0, 1.0, 1.0));
        outClr += getSpecularComponent(norm, lightDir, viewDir, powVal) * multVal;
        outClr += getFinalColor(norm, lightDir);
        lightDir = normalize(vec3(0.0, -1.0, 1.0));
        outClr += getSpecularComponent(norm, lightDir, viewDir, powVal) * multVal;
        outClr += getFinalColor(norm, lightDir);
        
        outClr = normalize(vec2(outClr * 0.2, 4.0)).x;
        
        outFragColor = vec4(vec3(outClr) * gelClr * pow((1.0 - multy) * (1.0 - multx), 0.5), pow(1.0 - pos.y, 0.3) * sharedUboData.data.x * 5.0 * (0.5 - lastVal));
        //outFragColor = vec4(viewDir, 1.0);
    }
    else
    {
        outFragColor = vec4(0.0);
    }
}