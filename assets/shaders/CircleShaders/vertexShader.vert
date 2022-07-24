#version 410 core

layout (location = 0) in vec2 vertex;

out vec2 localPos;
out vec4 worldPos;
out vec2 texturePosition;

uniform vec2 xSides;
uniform vec2 ySides;

uniform mat4 ortho;
uniform mat4 model;

uniform bool isPoint;
uniform bool isMirrored;

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
    worldPos = model * vec4(mPos, 0, 1);

    float xLen = xSides.y - xSides.x;
    float yLen = ySides.x - ySides.y;
    texturePosition = vec2((mPos.x - xSides.x) / xLen, (mPos.y - ySides.x) / yLen);
    if (isMirrored) texturePosition.x = 1 - texturePosition.x;

    gl_Position = ortho * model * vec4(mPos, 0.0, 1.0);
}