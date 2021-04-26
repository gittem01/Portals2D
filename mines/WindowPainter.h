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
	GLFWwindow* window;
	glm::vec2 windowSizes = glm::vec2(1280, 720);
	Camera* cam;
	float currentColor[4] = {1, 1, 1, 1};
	bool releaseQueue[3] = {false, false, false};
	float currentDepth = 0;
	bool isPicking, drawMode, grid;

	WindowPainter(Camera* cam);

	void massInit();
	bool looper();
	void clearMouseData();

	static void mouseEventCallback(GLFWwindow*, double, double);
	static void buttonEventCallback(GLFWwindow*, int, int, int);
	static void scrollEventCallback(GLFWwindow*, double, double);
	static void glfwKeyEventCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void windowSizeEventCallback(GLFWwindow*, int, int);
	static void glfwWindowFocusCallback(GLFWwindow*, int);
};