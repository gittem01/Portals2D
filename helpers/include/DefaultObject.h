#pragma once

#include <PortalWorld.h>
#include <PortalBody.h>
#include <Sounder.h>

class DefaultObject
{
public:
	std::vector<PortalBody*>* pBody;
	PortalWorld* pWorld;
	
	std::vector<ALuint> onHitSoundBuffers;

	DefaultObject(PortalWorld* portalWorld);
	DefaultObject(PortalWorld* portalWorld, PortalBody* pb);

	virtual void preSolve(b2Contact* contact, b2Body* thisBody, b2Fixture* fix2);
	virtual void reportContact(b2Contact* contact, const b2ContactImpulse* impulse, b2Body* contactingBody, b2Fixture* fix2);
};