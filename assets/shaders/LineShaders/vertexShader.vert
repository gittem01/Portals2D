#version 410 core

#define PI 3.141592653589793238

layout (location = 0) in vec2 vertex;

out vec2 localPos;
out vec2 size;

uniform mat4 ortho;

uniform float thickness;
uniform vec2 p1;
uniform vec2 p2;
uniform float zoom;

vec2 rotateAroundOrigin(vec2 point, float angle)
{
    float s = sin(angle);
    float c = cos(angle);

    vec2 p;

    p.x = point.x * c - point.y * s;
    p.y = point.y * c + point.x * s;

    return p;
}

void main()
{
    vec2 diffVec = p2 - p1;
    vec2 midPoint = (p1 + p2) * 0.5f;
    float dist = length(diffVec);
    float angle;

    // scale, rotate, transform
    vec2 vCopy = vertex;
    // scale
    vCopy.x *= dist;
    vCopy.y *= thickness / (zoom * zoom);

    // rotate
    if (diffVec.x == 0){
        angle = PI / 2;
    }
    else{
        angle = atan(diffVec.y / diffVec.x);
    }

    localPos = vCopy;
    size = vec2(dist * 0.5f, (thickness / (zoom * zoom)) * 0.5f);

    vCopy = rotateAroundOrigin(vCopy, angle);
    // transform
    vCopy += midPoint;



    gl_Position = ortho * vec4(vCopy, 0.0, 1.0);
}