#pragma once

#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>

class WindowHandler;

class Camera
{
public:
	glm::vec2 pos;
	float rotation;

	glm::vec2 defaultXSides = glm::vec2(-8.0f, +8.0f);
	glm::vec2 defaultYSides = glm::vec2(-4.5f, +4.5f);

	glm::vec2 xSides;
	glm::vec2 ySides;

	float baseX = 16.0f;

	glm::vec2 zoomLimits = glm::vec2(0.5, 10);

	glm::mat4 camMatrix;

	SDL_Window* window;
	float zoom = 0.5;
	float zoomInc = 0.15;
	float dragSmth = 0.2f;
	float neededZoom = 0;
	glm::vec2 zoomPoint;
	glm::vec2 lastPos = glm::vec2(-1, -1);
	glm::vec2 dragTo = glm::vec2(0, 0);

	WindowHandler* windowHandler;
	static std::vector<Camera*> cameras;

	Camera(glm::vec2 pos, WindowHandler* wh);
	Camera();
	~Camera();

	void init(glm::vec2 pos, WindowHandler* wh);

	void update();
	glm::vec2 getCameraCoords(glm::vec2 p);
	glm::vec2 getMouseCoords();
	void updateOrtho();
	void changeZoom(float inc);
	float limitZoom(float inZoom);
	void dragFunc(int width, int height);
};

