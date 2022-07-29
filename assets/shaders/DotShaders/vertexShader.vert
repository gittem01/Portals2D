#version 410 core

layout (location = 0) in vec2 vertex;
layout (location = 1) in vec2 position;
layout (location = 2) in vec2 size;
layout (location = 3) in vec3 mults;
layout (location = 4) in vec4 colourL;

out vec4 colour;
out vec2 localPositions;

out float inPerct;
out float outCurve;

uniform mat4 ortho;
uniform float zoom;

void main()
{
    colour = colourL;
    localPositions = vertex;
    
    inPerct = min(mults.y / (zoom * zoom), 0.1f);
    outCurve = mults.z;

    mat4 mOrtho = ortho;
    mOrtho[3][0] *= mults.x;
    mOrtho[3][1] *= mults.x;
    mOrtho[0][0] *= 1 / mults.x;
    mOrtho[1][1] *= 1 / mults.x;

    vec2 v = vec2(vertex.x * size.x, vertex.y * size.y);
    gl_Position = mOrtho * vec4(v + position, 0.0, 1.0);
}
