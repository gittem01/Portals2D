#pragma once

#include <DefaultObject.h>

class DefaultCube : public DefaultObject
{
public:
	DefaultCube(PortalWorld* portalWorld);
	void reportContact(b2Contact* contact, const b2ContactImpulse* impulse, b2Body* contactingBody, b2Fixture* fix2);
};