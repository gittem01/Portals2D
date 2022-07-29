#version 410 core

#define PI 3.14159265359

layout (location = 0) in vec2 vertex;
layout (location = 5) in float size;
layout (location = 6) in int side;
layout (location = 7) in float speed;
layout (location = 8) in vec4 mults;
layout (location = 9) in vec4 colourL;
layout (location = 10) in vec4 portal;

out vec4 colour;
out vec2 localPositions;
out vec2 worldPos;
out vec2 center;
out vec4 sPortal;
out float sSide;

out float inPerct;
out float outCurve;
out float frac;

uniform mat4 ortho;
uniform float zoom;
uniform float gameTime;
uniform int numParticles;

float fmod(float x, float y)
{
  return x - y * int(x / y);
}

vec2 rotateVec(vec2 vec, float angle){
    float x = cos(angle) * vec.x - sin(angle) * vec.y;
    float y = sin(angle) * vec.x + cos(angle) * vec.y;

    return vec2(x, y);
}

void main()
{
    colour = colourL;

    localPositions = vertex;
    
    inPerct = mults.y;
    outCurve = mults.z;

    float gt = fmod(gameTime * speed, mults.w);
    frac = 1 - gt / mults.w;

    int div = int((gameTime * speed) / mults.w);

    vec2 dir = portal.zw - portal.xy;
    float pSpaceDist = length(dir);
    float pMargin = size / pSpaceDist;
    vec2 dir90 = normalize(rotateVec(dir, PI / 2.0));
    vec2 pos = (portal.zw + portal.xy) * 0.5f + dir * (fmod((mults.x + (div * 0.05f * mults.x)), (1 - pMargin) * 0.5f));

    vec2 v = vertex * size;
    pos = pos + dir90 * gt;
    worldPos = v + pos;
    center = pos;
    sSide = side;
    sPortal = portal;

    gl_Position = ortho * vec4(v + pos, 0.0, 1.0);
}
