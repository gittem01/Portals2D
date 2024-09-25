#pragma once

#include <DefaultObject.h>

class Gun : public DefaultObject
{
private:
	double lastSoundTime = 0.0;
public:
	Gun(PortalWorld* portalWorld, b2Vec2 pos, b2Vec2 gunSize);
	void beginContact(b2Contact* contact, b2Fixture* otherFix, b2Fixture* thisFix);
	void preSolve(b2Contact* contact, b2Body* thisBody, b2Fixture* fix2);
};