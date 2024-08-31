#pragma once

#include <DefaultObject.h>

class LaserCube : public DefaultObject
{
private:
	double lastSoundTime = 0.0;
public:
	LaserCube(PortalWorld* portalWorld, b2Vec2 pos);
	void reportContact(b2Contact* contact, const b2ContactImpulse* impulse, b2Body* contactingBody, b2Fixture* fix2);
};