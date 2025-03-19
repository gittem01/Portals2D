#pragma once

#include <DefaultObject.h>

class Dropper;

class Sphere : public DefaultObject
{
private:
	double lastSoundTime = 0.0;
	Dropper* linkedDropper;
	bool startMirrored;
public:
	Sphere(PortalWorld* portalWorld, b2Vec2 pos, Dropper* linkedDropper=nullptr, bool mirrored=false);

	void recreate(bool mirrored = false);
	void reportContact(b2Contact* contact, const b2ContactImpulse* impulse, b2Body* contactingBody, b2Fixture* fix2);
};