#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class WindowHandler;

class Camera
{
public:
	glm::vec3 pos;
	glm::vec3 rotation;

	glm::vec3 lookDir;
	glm::vec3 rightDir;
	glm::vec3 upDir;

	glm::vec2 defaultXSides = glm::vec2(-8.0f, +8.0f);
	glm::vec2 defaultYSides = glm::vec2(-4.5f, +4.5f);

	glm::vec2 xSides;
	glm::vec2 ySides;

	float baseX = 16.0f;

	glm::vec2 zoomLimits = glm::vec2(0.55, 20);

	glm::mat4 pers;
	glm::mat4 view;

	glm::mat4 camMatrix;

	GLFWwindow* window;
	float zoom = 1.0;
	float zoomInc = 0.15;
	float dragSmth = 0.2f;
	float neededZoom = 0;
	glm::vec2 zoomPoint;
	glm::vec2 lastPos = glm::vec2(-1, -1);
	glm::vec2 dragTo = glm::vec2(0, 0);

	WindowHandler* windowHandler;

	Camera(glm::vec3 pos, WindowHandler* wh);
	void update();
	glm::vec2 getCameraCoords(glm::vec2 p);
	glm::vec2 getMouseCoords();
	void updateOrtho();
	void updateVectors();
	void changeZoom(float inc);
	float limitZoom(float inZoom);
	void dragFunc(int width, int height);
};

