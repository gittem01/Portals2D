#version 410 core

uniform mat4 model;
uniform mat4 ortho;

uniform vec2 xSides;
uniform vec2 ySides;
uniform vec2[8] vertices;

uniform bool isMirrored;

out vec2 localPos;
out vec4 worldPos;
out vec2 texturePosition;

void main()
{
    int pos;
    int id = gl_VertexID;
    if (id % 3 == 0){
        pos = 0;
    }
    else if (id % 3 == 1){
        pos = id / 3 + 1;
    }
    else{
        pos = id / 3 + 2;
    }

    vec2 v = vertices[pos];
    localPos = v;
    worldPos = model * vec4(v, 0.0, 1.0);

    float xLen = xSides.y - xSides.x;
    float yLen = ySides.x - ySides.y;
    texturePosition = vec2((v.x - xSides.x) / xLen, (v.y - ySides.x) / yLen);
    if (isMirrored) texturePosition.x = 1 - texturePosition.x;

    gl_Position = ortho * model * vec4(v, 0.0, 1.0);
}