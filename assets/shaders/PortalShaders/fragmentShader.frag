#version 410 core

out vec4 FragColor;

in vec4 colour;
in vec4 sPortal;
in vec2 worldPos;
in vec2 center;
in float sSide;
in vec2 localPositions;

in float inPerct;
in float outCurve;
in float frac;

float lineRel(vec2 a, vec2 b, vec2 c){
     return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

void main()
{
    float l = length(localPositions);
    if (l > 0.5){
        FragColor = vec4(0, 0, 0, 0);
    }
    else{
        if (lineRel(sPortal.zw, sPortal.xy, worldPos) > 0){
            FragColor = vec4(0, 0, 0, 0);
        }
        else{
            if (l < inPerct){
                FragColor = vec4(colour.xyz, frac);
            }
            else{
                float rest = 0.5 - inPerct;
                float t = (rest - (l - inPerct)) / rest;
                t = pow(t, outCurve);
                FragColor = vec4(colour.xyz, t * frac);
            }
        }
    }
}