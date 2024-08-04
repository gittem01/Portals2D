#version 460 core

layout (location = 0) out vec4 FragColour;

layout (location = 0) flat in ivec4 inGelData[16];
layout (location = 16) in vec2 wp;

layout (set = 0, binding = 0) uniform SharedUboData
{
    mat4 cameraMatrix;
    vec4 cameraPos;
    vec4 data;
} sharedUboData;

layout (push_constant) uniform PC
{    
    vec4 fData; // gridSize, none, none, none
    ivec4 iData; // swapchainImage, gelType, numSquares, arrStart
    ivec4 iData2; // xSquareCount, ySquareCount, none, none
} gelData;

struct storageData
{
    vec4 posR; // posx, posy, radius, none
    vec4 color;
};

layout (std140, set = 1, binding = 0) readonly buffer UBO
{
    storageData sData[];
} ubo;

void main()
{
    int numGels = inGelData[0][0];

    float smoothThick = 0.1 / sharedUboData.data.x;

    vec2 vecSum = vec2(0.0, 0.0);
    float lengthSum = 0.0f;
    float adjV = 5.0;
    vec4 endColor = vec4(0.0);
    float extra = 0.0;
    for (int i = 0; i < numGels; i++)
    {
        storageData gel = ubo.sData[inGelData[i / 4 + 1][i % 4]];
        vec4 posR = gel.posR;
        vec2 diff = wp - posR.xy;
        vec2 normDiff = normalize(diff);
        vec2 lengthAdjusted = (normDiff / pow(length(diff), adjV)) * pow(posR.z, adjV);
        vecSum += lengthAdjusted;
        lengthSum += length(lengthAdjusted);
        
        float m = max(pow((posR.z - length(diff)) / posR.z, 3.0), 0.0);
        endColor += gel.color * m;
        extra += (1.0 - gel.color.a) * m;
    }

    vec4 gelColor = vec4(normalize(endColor.xyz), normalize(vec2(endColor.a, extra)).x);
    
    float dist = 1.0 - 1.0 / pow(lengthSum, 1.0 / adjV);
    float normalizedDist = (dist - 0.5) * 2.0;

    if (normalizedDist > 0.0)
    {
        float upDist = sqrt(1.0 - pow(1.0 - normalizedDist, 2.0));
        float ratio = length(vecSum) / lengthSum;
        vec2 normVec = vec2(ratio * normalize(vecSum));
        vec3 normal = vec3((1.0 - normalizedDist) * normalize(vecSum), upDist) * ratio;
        normal += (1.0 - ratio * ratio) * normalize(vec3(vecSum * ratio * (1.0 - upDist * upDist), lengthSum * upDist * upDist));
        
        normal = normalize(normal);

        vec3 worldPos = vec3(wp, 0.0);
        vec3 viewDir = normalize(worldPos - vec3(sharedUboData.cameraPos.xy * vec2(1.0, -1.0), sharedUboData.cameraPos.z));
        vec3 lightDir = normalize(vec3(-1.0, 1.0, -1.0));
        vec3 reflectDir = reflect(-lightDir, normal);

        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
        float diff = dot(normal, -lightDir) + 0.5;
        diff = pow(diff, 2.0) * 0.5;
        
        float specular = 1.0 * spec;
        vec4 requestedColor = (specular * gelColor.xyz, specular) + vec4(diff * gelColor.xyz + gelColor.xyz * 0.2, gelColor.w);
        
        float normNorm = smoothstep(normalizedDist / smoothThick, 0.0, 0.1);
        FragColour = requestedColor * normNorm;
    }
    else
    {
        FragColour = vec4(0.0);
    }
}