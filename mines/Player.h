#pragma once

#include "Shape.h"
#include "WindowPainter.h"
#include "Gun.h"

class Player {
public:
	Gun* gun;
	Shape* shape;
	WindowPainter* wp;
	float radius = 0.5f;
	bool portalCollision = false;
	Player(b2World* world, b2Vec2 pos, WindowPainter* wp);
	void swapShape(Shape* newShape);
	void update();
	void handleInput();
};