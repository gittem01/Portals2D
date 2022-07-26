#version 410 core

out vec4 FragColor;

in vec4 colour;
in vec2 localPositions;

in float inPerct;
in float outCurve;

void main()
{
    float l = length(localPositions);
    if (l > 0.5){
        FragColor = vec4(0, 0, 0, 0);
    }
    else{
        if (l < inPerct){
            FragColor = vec4(colour.xyz, 1.0);
        }
        else{
            float rest = 0.5 - inPerct;
            float t = (rest - (l - inPerct)) / rest;
            t = pow(t, outCurve);
            FragColor = vec4(colour.xyz, t);
        }
    }
}