#version 410 core

out vec4 color;

in vec2 localPos;
in vec2 size;

uniform vec4 colour;

uniform float zoom;

float lineRel(vec2 a, vec2 b, vec2 c){
     return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

void main()
{
    vec2 p1 = vec2(size.x - size.y, 0.0f);
    vec2 p2 = vec2(-size.x + size.y, 0.0f);
    if (localPos.x < p2.x && length(localPos - p2) > size.y || localPos.x > p1.x && length(localPos - p1) > size.y){
        color = vec4(0, 0, 0, 0);
        return;
    }
    color = colour;
}