#version 460 core

float edgeData[80];

layout (location = 0) out vec4 outFragColor;

layout (location = 0) in vec2 localPosition;
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
	vec4 positions;
} pc;

float getFinalColor(vec3 normal, vec3 lightDir)
{
    float NdotL = max(dot(normalize(normal), normalize(lightDir)) + 1.0, 0.0);
    return NdotL;
}

float getSpecularComponent(vec3 normal, vec3 lightDir, vec3 viewDir, float shininess) {
    vec3 N = normalize(normal);
    vec3 L = normalize(lightDir);
    vec3 V = normalize(viewDir);
    vec3 R = reflect(-L, N);

    float RdotV = max(dot(R, V), 0.0);
    return pow(RdotV, shininess);
}

float rand(vec2 seed)
{
    return fract(sin(dot(seed, vec2(13.8695, 17.1371))) * 73714.38361);
}

float opSmoothUnion(float d1, float d2, float k)
{
    float h = max(k-abs(d1-d2),0.0);
    return min(d1, d2) - h*h*0.25/k;
}

void main()
{
    const float perTileGel = 10.0;
    float iTime = sharedUboData.data.y;
    vec3 gelClr = vec3(0.0, 0.5, 1.5);
    float xSize = 8.0;
    for (int i = 0; i < 80; i++)
	{
		edgeData[i] = sin(i * 1000.0) * 0.3 + 0.2 + 0.05 * sin(iTime * rand(vec2(i))) - 0.15;
        edgeData[i] = 0.6 + rand(vec2(i)) * 0.15 + 0.05 * sin(iTime * rand(vec2(i)));
        //if (edgeData[i] < 0.2) edgeData[i] = -0.2;
	}

	vec2 pos = localPosition;
    vec3 norm = vec3(0.0, 0.0, 0.0);
    float lastVal = 1.0;
    for (int i = -2; i < 2; i++)
    {
        float checkX = float(round(pos.x * perTileGel + 0.5)) + float(i);
        float val = float(edgeData[int(checkX)]);
        vec2 diff = vec2(pos.x * perTileGel + 0.5, pos.y * 0.4 * perTileGel) - vec2(checkX, min(val, pos.y) * 0.4 * perTileGel);
        float revDiff = pow(max(1.0 - length(diff), 0.0), 2.0);
        norm += normalize(vec3(diff, revDiff * 3.0)) * revDiff;
        lastVal = opSmoothUnion(lastVal, pow(length(diff), 2.0), 0.7);
    }

    const float vx = 0.1;
    const float vy = vx * 2.5;
    float multy = (max(vy - pos.y, 0.0)) / vy;
    float xVal = pos.x;
    if (pos.x > 0.5)
    {
        xVal = xSize - pos.x;
    }

    float multx = (max(vx - xVal, 0.0)) / vx;

    norm = normalize(norm);
    
    if (lastVal < 0.5)
    {        
        vec3 lightDir = vec3(1.0, 1.0, 1.0);
        float outClr = getSpecularComponent(norm, lightDir, vec3(0.0, 0.0, 1.0), 75.0) * 100.0;
        outClr += getFinalColor(norm, lightDir);
        outClr = normalize(vec2(outClr, 3.0)).x;
        
        outFragColor = vec4(vec3(outClr) * gelClr * pow((1.0 - multy) * (1.0 - multx), 0.5), sharedUboData.data.x * 5.0 * pow(0.5 - lastVal, 1.0));
        //outFragColor = vec4(norm, 1.0);
    }
    else
    {
        outFragColor = vec4(0.0);
    }
}