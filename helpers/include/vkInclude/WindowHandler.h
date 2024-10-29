#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include <SDL3/SDL.h>
class Camera;

class WindowHandler
{
public:
#ifdef __APPLE__
	static const int pIndLeftCommand = SDL_SCANCODE_LGUI;
	static const int pIndRightCommand = SDL_SCANCODE_RGUI;
#else
	static const int pIndLeftCommand = SDL_SCANCODE_LCTRL;
	static const int pIndRightCommand = SDL_SCANCODE_RCTRL;
#endif // APPLE



	int* keyData = (int*)calloc(512, sizeof(int));
	double* mouseData = (double*)calloc(7, sizeof(double));
	double* trackpadData = (double*)calloc(2, sizeof(int));
	int* lastMousePos = (int*)calloc(2, sizeof(int));
	glm::vec2 windowSizes = glm::vec2(1280, 720);
	std::vector<int> newPressIndices;
	SDL_Window* window;
	Camera* cam;

	uint32_t dpiScaling = 1;

	WindowHandler(Camera* cam);

	void massInit();
	bool handleEvents();
	bool looper();
	void handleMouseData();
	void handleKeyData();
};