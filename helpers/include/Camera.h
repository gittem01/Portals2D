#pragma once

#include <Renderer.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class Camera
{
public:
	glm::vec2 pos;

	glm::vec2 defaultXSides = glm::vec2(-8.0f, +8.0f);
	glm::vec2 defaultYSides = glm::vec2(-4.5f, +4.5f);
	
	glm::vec2 xSides;
	glm::vec2 ySides;

	float baseX = 16.0f;

	glm::vec2 zoomLimits = glm::vec2(0.25, 2);

	glm::mat4 ortho;

	GLFWwindow* window;
	float zoom = 0.8;
	float zoomInc = 0.2;
	float dragSmth = 0.5f;
	float neededZoom = 0;
	glm::vec2 zoomPoint;
	glm::vec2 lastPos = glm::vec2(-1, -1);
	glm::vec2 dragTo = glm::vec2(0, 0);

	void* wh;

	Camera(glm::vec2 pos, void* wh);
	void update();
	glm::vec2 getCameraCoords(glm::vec2 p);
	glm::vec2 getMouseCoords();
	void updateOrtho();
	void changeZoom(float inc);
	float limitZoom(float inZoom);
	void dragFunc(int width, int height);
	void cursorOutFunc(int width, int height);
};

