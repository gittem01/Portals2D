#version 410 core

out vec4 color;

in vec2 localPos;
in vec4 worldPos;

uniform vec4 colour;
uniform vec4 outerColour;

uniform bool isFullRelease;
uniform bool drawReleases;
uniform vec4 releaseColour;

uniform float lineDist;
uniform float zoom;
uniform float size;

uniform bool isPoint;

uniform vec4[256] portals;
uniform int[256] sideMults;
uniform int numPortals;

float lineRel(vec2 a, vec2 b, vec2 c){
     return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

void main()
{
    if (isFullRelease && !drawReleases){
        color = vec4(0, 0, 0, 0);
        return;
    }

    bool isReleaseDraw = false;
    if (!isPoint && !isFullRelease){
        for (int i = 0; i < numPortals; i++){
            if (lineRel(portals[i].xy, portals[i].zw, worldPos.xy) * sideMults[i] < 0){
                if (!drawReleases){
                    color = vec4(0, 0, 0, 0);
                    return;
                }
                else{
                    isReleaseDraw = true;
                    break;
                }
            }
        }
    }

    float lVec = length(localPos);
    float r = size / 2;
    if (isPoint) r /= (zoom * zoom);
    float adjustedThickness = lineDist / (zoom * zoom);
    if (lVec > r){
        color = vec4(0, 0, 0, 0);
    }
    else if (isPoint){
        color = colour;
    }
    else{
        if (lVec > r - adjustedThickness){
            if (isReleaseDraw || isFullRelease){
                color = vec4(vec3(releaseColour), releaseColour.w * 2.0f);
            }
            else{
                color = outerColour;
            }
        }
        else{
            if (isReleaseDraw || isFullRelease){
                color = releaseColour;
            }
            else{
                color = vec4(colour.xyz, colour.w * 0.5f);
            }
        }
    }
}