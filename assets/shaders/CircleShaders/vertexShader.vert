#version 410 core

layout (location = 0) in vec2 vertex;

out vec2 localPos;
out vec4 worldPos;

uniform mat4 ortho;

uniform bool isPoint;

uniform float zoom;
uniform float size;
uniform vec2 pos;

void main()
{
    float val;
    
    vec2 mPos;
    if (!isPoint){
         mPos = vertex * size + pos;
    }
    else{
        mPos = vertex * (size / (zoom * zoom)) + pos;
    }
    localPos = mPos - pos;
    worldPos = vec4(mPos, 0.0, 1.0);

    gl_Position = ortho * vec4(mPos, 0.0, 1.0);
}