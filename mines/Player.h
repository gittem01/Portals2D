#pragma once

#include "Shape.h"
#include "WindowPainter.h"

class Player {
public:
	Shape* shape;
	WindowPainter* wp;
	float radius = 0.5f;
	Player(b2World* world, b2Vec2 pos, WindowPainter* wp);
	void swapShape(Shape* newShape);
	void update();
	void handleInput();
};