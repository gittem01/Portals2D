#pragma once

#include <box2d/box2d.h>

class polygon
{
private:
	b2World* world;
	b2Vec2 pos;

public:
	b2Body* body;

	polygon(b2World* world, b2Vec2 pos);
	void createBox(b2Vec2 size, b2BodyType bodyType);
};

