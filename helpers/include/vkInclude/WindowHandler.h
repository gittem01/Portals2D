#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

class Camera;

class WindowHandler
{
public:
#ifdef __APPLE__
	static const int pIndLeftCommand = GLFW_KEY_LEFT_SUPER;
	static const int pIndRightCommand = GLFW_KEY_RIGHT_SUPER;
#else
	static const int pIndLeftCommand = GLFW_KEY_LEFT_CONTROL;
	static const int pIndRightCommand = GLFW_KEY_RIGHT_CONTROL;
#endif // APPLE



	int* keyData = (int*)calloc(512, sizeof(int));
	double* mouseData = (double*)calloc(7, sizeof(double));
	double* trackpadData = (double*)calloc(2, sizeof(int));
	int* lastMousePos = (int*)calloc(2, sizeof(int));
	glm::vec2 windowSizes = glm::vec2(1280, 720);
	bool releaseQueue[3] = { false, false, false };
	std::vector<int> newPressIndices;
	GLFWwindow* window;
	Camera* cam;

	uint32_t dpiScaling = 1;

	WindowHandler(Camera* cam);

	void massInit();
	bool looper();
	void handleMouseData();
	void handleKeyData();

	static void mouseEventCallback(GLFWwindow*, double, double);
	static void buttonEventCallback(GLFWwindow*, int, int, int);
	static void scrollEventCallback(GLFWwindow*, double, double);
	static void glfwKeyEventCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void windowSizeEventCallback(GLFWwindow*, int, int);
	static void glfwWindowFocusCallback(GLFWwindow*, int);
};