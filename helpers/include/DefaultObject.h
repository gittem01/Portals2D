#pragma once

#include <PortalWorld.h>
#include <PortalBody.h>
#include <Sounder.h>
#include <bitset>
#include "ThinDrawer.h"

class ForceField;
class Funnel;

class DefaultObject
{
public:
	PbWorldData* wData;
	g_PhysicsData pData;
	PortalWorld* pWorld;
	s_renderGroup* renderGroup = nullptr;
	std::vector<ALuint> onHitSoundBuffers;
	b2Vec2 initialPos;
	std::bitset<MAX_FUNNELS * 2> funnelMasks;
	std::bitset<MAX_FUNNELS * 2> newFunnelMasks;
	Funnel* effectiveFunnel;
	bool inPortalFunel;

	static std::vector<DefaultObject*> defaultObjects;

	DefaultObject() {}
	DefaultObject(PortalWorld* portalWorld);
	DefaultObject(PortalWorld* portalWorld, PortalBody* pb);

	void paintedBounce(b2WorldManifold& wManifold, b2Body* body);
	b2Vec2 tryBounce(b2Contact* contact, b2Fixture* thisFix, b2Fixture* otherFix);
	virtual void enterForceField(ForceField* forceField);
	virtual void recreate(bool mirrored=false);
	virtual void beginContact(b2Contact* contact, b2Fixture* otherFix, b2Fixture* thisFix);
	virtual void endContact(b2Contact* contact, b2Fixture* otherFix);
	virtual void preSolve(b2Contact* contact, b2Body* thisBody, b2Fixture* fix2);
	virtual void reportContact(b2Contact* contact, const b2ContactImpulse* impulse, b2Body* contactingBody, b2Fixture* fix2);
};