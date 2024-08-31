#pragma once

#include <PortalWorld.h>
#include <PortalBody.h>
#include <Sounder.h>

class DefaultObject
{
public:
	PbWorldData* wData;
	g_PhysicsData* pData;
	PortalWorld* pWorld;
	
	b2Vec2 initialPos;

	static std::vector<DefaultObject*> defaultObjects;
	std::vector<ALuint> onHitSoundBuffers;

	DefaultObject(PortalWorld* portalWorld);
	DefaultObject(PortalWorld* portalWorld, PortalBody* pb);

	virtual void recreate();
	virtual void beginContact(b2Contact* contact, b2Fixture* otherFix);
	virtual void endContact(b2Contact* contact, b2Fixture* otherFix);
	virtual void preSolve(b2Contact* contact, b2Body* thisBody, b2Fixture* fix2);
	virtual void reportContact(b2Contact* contact, const b2ContactImpulse* impulse, b2Body* contactingBody, b2Fixture* fix2);
};