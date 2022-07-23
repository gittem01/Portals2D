#version 410 core

uniform mat4 model;
uniform mat4 ortho;

uniform vec2[8] vertices;

out vec2 localPos;
out vec4 worldPos;

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

    localPos = vertices[pos];
    worldPos = model * vec4(vertices[pos], 0.0, 1.0);

    gl_Position = ortho * model * vec4(vertices[pos], 0.0, 1.0);
}