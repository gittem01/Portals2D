#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include "Camera.h"

class WindowPainter
{
public:
	int* mouseData = (int*)calloc(7, sizeof(int));
	int* lastMousePos = (int*)calloc(2, sizeof(int));
	SDL_Window* window;
	glm::vec2 lastWindowSizes = glm::vec2(1600, 900);
	Camera* cam;
	float currentColor[4] = {1, 1, 1, 1};
	bool releaseQueue[3] = {false, false, false};
	float currentDepth = 0;
	bool isPicking, drawMode, grid;

	WindowPainter(Camera* cam);

	void massInit();
	bool looper();
	void clearMouseData();
};