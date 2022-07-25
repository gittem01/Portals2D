#include "Camera.h"
#include "WindowPainter.h"

Camera::Camera(glm::vec2 pos, void* wh) {
	WindowPainter* windowHandler = (WindowPainter*)wh;

	this->pos = pos;
	this->wh = wh;
	this->window = windowHandler->window;

	xSides = this->defaultXSides / (this->zoom * this->zoom) + this->pos.x;
	ySides = this->defaultYSides / (this->zoom * this->zoom) + this->pos.y;

	updateOrtho();
}

void Camera::updateOrtho() {
	xSides = this->defaultXSides / (this->zoom * this->zoom) + this->pos.x;
	ySides = this->defaultYSides / (this->zoom * this->zoom) + this->pos.y;
	
	this->ortho = glm::ortho(xSides.x, xSides.y, ySides.x, ySides.y, -100.0f, +100.0f);
}

void Camera::changeZoom(float inc) {
	glm::vec2 mp = this->getCameraCoords(this->zoomPoint);

	float zoomAfter = this->limitZoom(zoom + inc);

	glm::vec2 xSidesAfter = this->defaultXSides / (zoomAfter * zoomAfter) + this->pos.x;
	glm::vec2 ySidesAfter = this->defaultYSides / (zoomAfter * zoomAfter) + this->pos.y;

	float xPerctBefore = (mp.x - this->pos.x) / (xSides.y - xSides.x);
	float xPerctAfter = (mp.x - this->pos.x) / (xSidesAfter.y - xSidesAfter.x);

	float yPerctBefore = (mp.y - this->pos.y) / (ySides.y - ySides.x);
	float yPerctAfter = (mp.y - this->pos.y) / (ySidesAfter.y - ySidesAfter.x);

	this->pos.x += (xPerctAfter - xPerctBefore) * (xSidesAfter.y - xSidesAfter.x);
	this->pos.y += (yPerctAfter - yPerctBefore) * (ySidesAfter.y - ySidesAfter.x);
	this->zoom += inc;
	this->zoom = this->limitZoom(zoom);
}

float Camera::limitZoom(float inZoom) {
	if (inZoom < zoomLimits.x) {
		inZoom = zoomLimits.x;
	}
	else if (inZoom > zoomLimits.y) {
		inZoom = zoomLimits.y;
	}
	return inZoom;
}

void Camera::update() {
	WindowPainter* windowHandler = (WindowPainter*)wh;

	int width, height;
	glfwGetWindowSize(this->window, &width, &height);

	this->dragFunc(width, height);
	if (windowHandler->mouseData[5] != 0 && 
		!windowHandler->keyData[GLFW_KEY_LEFT_CONTROL] &&
		!windowHandler->keyData[GLFW_KEY_RIGHT_CONTROL] &&
		!windowHandler->keyData[GLFW_KEY_LEFT_SHIFT])
	{
		this->neededZoom += zoom * windowHandler->mouseData[5] / 10.0f;
		this->zoomPoint.x = windowHandler->mouseData[0]; this->zoomPoint.y = height - windowHandler->mouseData[1];
	}

	if (this->neededZoom != 0) {
		this->changeZoom(this->zoomInc * this->neededZoom);
		this->neededZoom -= zoomInc * this->neededZoom;

	}

	updateOrtho();
}

void Camera::dragFunc(int width, int height) {
	WindowPainter* windowHandler = (WindowPainter*)wh;

	glm::vec2 diffVec = glm::vec2(dragTo.x - lastPos.x,
		dragTo.y - lastPos.y);
	glm::vec2 sideDiffs = glm::vec2(defaultXSides.y - defaultXSides.x,
		defaultYSides.y - defaultYSides.x);

	float lng = glm::length(diffVec);

	if (abs(lng) > 0) {
		pos.x -= diffVec.x * (sideDiffs.x / width) * dragSmth / (zoom * zoom);
		pos.y += diffVec.y * (sideDiffs.y / height) * dragSmth / (zoom * zoom);
		
		lastPos.x += diffVec.x * dragSmth;
		lastPos.y += diffVec.y * dragSmth;
	}

	if (windowHandler->keyData[GLFW_KEY_LEFT_SHIFT]){
		pos.x -= windowHandler->trackpadData[0] * (sideDiffs.x / width) * dragSmth / (zoom * zoom) * 5.0f;
		pos.y += windowHandler->trackpadData[1] * (sideDiffs.y / height) * dragSmth / (zoom * zoom) * 5.0f;
	}
	else{
		if (windowHandler->mouseData[4] == 2) {
			lastPos.x = windowHandler->mouseData[0];
			lastPos.y = windowHandler->mouseData[1];

			dragTo.x = windowHandler->mouseData[0];
			dragTo.y = windowHandler->mouseData[1];
		}
		else if (windowHandler->mouseData[4] == 1) {
			dragTo.x = windowHandler->mouseData[0];
			dragTo.y = windowHandler->mouseData[1];
		}
	}
}

glm::vec2 Camera::getMouseCoords() {
	WindowPainter* windowHandler = (WindowPainter*)wh;

	int width, height;
	glfwGetWindowSize(this->window, &width, &height);
	float xPerct = windowHandler->mouseData[0] / (float)width;
	float yPerct = 1 - windowHandler->mouseData[1] / (float)height;

	glm::vec2 xSides = this->defaultXSides / (this->zoom * this->zoom) + this->pos.x;
	float xDiff = xSides.y - xSides.x;

	glm::vec2 ySides = this->defaultYSides / (this->zoom * this->zoom) + this->pos.y;
	float yDiff = ySides.y - ySides.x;

	float xPos = xSides.x + xPerct * xDiff;
	float yPos = ySides.x + yPerct * yDiff;

	return glm::vec2(xPos, yPos);
}

glm::vec2 Camera::getCameraCoords(glm::vec2 p) {
	int width, height;
	glfwGetWindowSize(this->window, &width, &height);
	float xPerct = p.x / width;
	float yPerct = p.y / height;

	glm::vec2 xSides = this->defaultXSides / (this->zoom * this->zoom) + this->pos.x;
	float xDiff = xSides.y - xSides.x;

	glm::vec2 ySides = this->defaultYSides / (this->zoom * this->zoom) + this->pos.y;
	float yDiff = ySides.y - ySides.x;

	float xPos = xSides.x + xPerct * xDiff;
	float yPos = ySides.x + yPerct * yDiff;

	return glm::vec2(xPos, yPos);
}