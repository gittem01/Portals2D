#pragma once

#include <box2d/box2d.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <set>

struct teleportData {
	b2Vec2 p1;
	b2Vec2 p2;
	float angle;
	b2Fixture* fixture;
};

enum dataTypes
{
	OTHER = 0,
	PORTAL = 1,
	SHAPE = 2,
	MOUSE = 3,
};

struct bodyData {
	dataTypes type;
	void* data;
};

class Shape
{
public:
	std::vector<b2Body*> addBodies; // add		bodies
	std::vector<b2Body*> dtrBodies; // destroy	bodies

	b2World* world;
	teleportData* data;

	float defaultLinearDamping = 0.3f;
	float defaultAngularDamping = 0.3f;
	float defaultDensity = 1.0f;
	float defaultrestitution = 0.3f;
	float defaultFriction = 0.5f;
	bool isBullet = false;

	bool isControllable = false;
	void* controlClass;

	b2Body* body;
	b2Vec2 pos;

	std::vector<b2Body*> bodies;

	Shape(b2World* world, b2Vec2 pos);
	~Shape();

	void createPolyFromData(teleportData* data);
	void createRect(b2Vec2 size, b2BodyType bodyType);

	void createCircleFromData(teleportData* data);
	void createCircle(float r, b2BodyType bodyType);

	void portalCollideStart(void* portal, b2Fixture* fix);
	void portalCollideEnd(void* portal, b2Fixture* fix, bool shouldDestroy);

	void creation();
	void destruction();

	void setData(teleportData* data);
	void applyData();
};