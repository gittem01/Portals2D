#pragma once

#include <box2d/box2d.h>
#include <stdlib.h>

struct teleportData {
	b2Vec2 p1;
	b2Vec2 p2;
	float angle;
	b2Fixture* fixture;
};


class Shape
{
public:
	b2World* world;
	teleportData* data;

	float defaultLinearDamping = 0.5f;
	float defaultAngularDamping = 0.5f;
	float defaultDensity = 1.0f;
	float defaultrestitution = 1.0f;
	float defaultFriction = 0.5f;
	bool isBullet = false;

	b2Body* body;
	b2Vec2 pos;

	Shape(b2World* world, b2Vec2 pos);

	void createPolyFromData(teleportData* data);
	void createRect(b2Vec2 size, b2BodyType bodyType);

	void createCircleFromData(teleportData* data);
	void createCircle(float r, b2BodyType bodyType);

	void setData(teleportData* data);
	void applyData();
};