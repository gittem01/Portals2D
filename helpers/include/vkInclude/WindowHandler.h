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


	int keyData[SDL_SCANCODE_COUNT] = { 0 };
	int repeatKeys[SDL_SCANCODE_COUNT] = { 0 };
	double mouseData[7] = {0};
	double trackpadData[2] = {0};
	int lastMousePos[2] = {0};
	glm::vec2 windowSizes = glm::vec2(1600, 900);
	std::vector<int> newPressIndices;
	std::vector<int> repeatClears;
	SDL_Window* window;
	Camera* cam;
	double lastMouseMovementTime = 0.0;
	std::string text;
	
	float dpiScaling = 1.0f;

	WindowHandler(Camera* cam);

	void massInit();
	bool handleEvents();
	bool looper();
	void handleMouseData();
	void handleKeyData();
};