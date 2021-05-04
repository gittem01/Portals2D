#pragma once

#include "Shape.h"
#include "WindowPainter.h"
#include "Gun.h"

class Player {
public:
	Gun* gun;
	Shape* shape;
	WindowPainter* wp;
	void* playerPortals[2];
	float radius = 0.3f;
	bool portalCollision = false;
	Player(b2World* world, b2Vec2 pos, WindowPainter* wp);
	void swapShape(Shape* newShape);
	void update();
	void handleInput();
};