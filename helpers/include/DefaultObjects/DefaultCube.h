#pragma once

#include <DefaultObject.h>

b2Body* createPortalCube2(b2Vec2 pos, b2Vec2 size, b2World* world, b2BodyType type, float density = 1.0f);

class DefaultCube : public DefaultObject
{
private:
	double lastSoundTime = 0.0;
public:
	DefaultCube(PortalWorld* portalWorld, b2Vec2 pos);
	void reportContact(b2Contact* contact, const b2ContactImpulse* impulse, b2Body* contactingBody, b2Fixture* fix2);
};