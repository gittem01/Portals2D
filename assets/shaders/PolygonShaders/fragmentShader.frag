#version 410 core

out vec4 color;

in vec2 localPos;
in vec4 worldPos;

uniform vec4 colour;
uniform vec4 outerColour;

uniform bool isFullRelease;
uniform bool drawReleases;
uniform vec4 releaseColour;

uniform vec2[8] vertices;
uniform int numPoints;
uniform float lineDist;
uniform float zoom;

uniform vec4[256] portals;
uniform int[256] sideMults;
uniform int numPortals;

// line goes from a to b
// c is the point
float getDist(vec2 a, vec2 b, vec2 c){
    float v1 = (b.x - a.x) * (a.y - c.y);
    float v2 = (a.x - c.x) * (b.y - a.y);
    float top = abs(v1 - v2);

    v1 = pow(b.x - a.x, 2);
    v2 = pow(b.y - a.y, 2);
    float bot = sqrt(v1 + v2);

    return top / bot;
}

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
    if (!isFullRelease){
        for (int i = 0; i < numPortals; i++){
            if (lineRel(portals[i].xy, portals[i].zw, worldPos.xy) * sideMults[i] < 0){
                if (!drawReleases){
                    color = vec4(0, 0, 0, 0);
                    return;
                }
                else{
                    color = releaseColour;
                    isReleaseDraw = true;
                    break;
                }
            }
        }
    }

    float ld = lineDist / (zoom * zoom);
    float smallestDist = 99999999999999.0;
    for (int i = 0; i < numPoints; i++)
    {
        float dist = getDist(vertices[i], vertices[(i + 1) % numPoints], localPos);
        if (dist < smallestDist) smallestDist = dist;
    }

    if (smallestDist < ld)
    {
        float dDist = ld - smallestDist;
        float prp = pow(dDist / ld, 30);
        float val;
        if (prp < 0){
            val = 1 + prp;
        }
        else{
            val = 1 - prp;
        }
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