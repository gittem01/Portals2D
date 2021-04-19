#pragma once

#include <box2d/box2d.h>
#include <stdlib.h>

struct teleportData {
	b2Transform transform;
	b2Vec2 linearVelocity;
	float angularVelocity;
	b2Fixture* fixture;
};

class polygon
{
private:
	b2World* world;
	teleportData* data;
public:
	b2Body* body;
	b2Vec2 pos;

	polygon(b2World* world, b2Vec2 pos);
	void createBox(b2Vec2 size, b2BodyType bodyType);
	void createShape(teleportData* data, b2BodyType bodyType);
	void setData(teleportData* data);
	void applyData();
};