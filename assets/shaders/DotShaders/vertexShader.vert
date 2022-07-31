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

uniform float zoom;

uniform vec2 camPos;
uniform vec2 camXSides;
uniform vec2 camYSides;

float fmod(float x, float y)
{
	return x - y * int(x / y);
}

void main()
{
	colour = colourL;
	localPositions = vertex;

	inPerct = min(mults.y / (zoom * zoom), 0.1f);
	outCurve = mults.z;

	vec2 posCopy = position;

	posCopy.x -= camPos.x * mults.x;
	posCopy.y -= camPos.y * mults.x;

	const float xCopySize = 16.0f * 16;
	const float yCopySize = 9.0f * 16;

	if (abs(posCopy.x) > xCopySize){
		if (posCopy.x > 0){
			posCopy.x = fmod(posCopy.x, xCopySize) - xCopySize * 0.5f;
		}
		else{
			posCopy.x = fmod(posCopy.x, xCopySize) + xCopySize * 0.5f;
		}
	}
	if (abs(posCopy.y) > yCopySize){
		if (posCopy.y > 0){
			posCopy.y = fmod(posCopy.y, yCopySize) - yCopySize * 0.5f;
		}
		else{
			posCopy.y = fmod(posCopy.y, yCopySize) + yCopySize * 0.5f;
		}
	}

	float xDiff = (camXSides.y - camXSides.x) * 0.5f;
	float yDiff = (camYSides.y - camYSides.x) * 0.5f;

	posCopy.x /= xDiff;
	posCopy.y /= yDiff;

	float sizeFracX = size.x / xDiff;
	float sizeFracY = size.y / yDiff;
	vec2 v = vertex * vec2(sizeFracX, sizeFracY) + posCopy;
	gl_Position = vec4(v, 0, 1);
}