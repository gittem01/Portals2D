#pragma once

#include <box2d/box2d.h>

class Gun
{
public:
	b2Vec2 pos;
	b2Vec2 targetPos;
	b2Vec2 currentNormal;
	b2Vec2 currentCollisionPos;
	b2Fixture* currentTarget;
	float currentFraction;
	b2World* world;

	Gun(b2Vec2 pos, b2Vec2 targetPos, b2World* world);
	~Gun();

	void update();
	void draw();

private:

};