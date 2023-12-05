#pragma once

#include <DefaultObject.h>

class Cyber : public DefaultObject
{
private:
	double lastSoundTime = 0.0;
public:
	Cyber(PortalWorld* portalWorld);
	void reportContact(b2Contact* contact, const b2ContactImpulse* impulse, b2Body* contactingBody, b2Fixture* fix2);
};