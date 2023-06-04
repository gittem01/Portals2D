#include "DefaultObject.h"

DefaultObject::DefaultObject(PortalWorld* portalWorld)
{
	this->pWorld = portalWorld;
}

DefaultObject::DefaultObject(PortalWorld* portalWorld, PortalBody* pb)
{
	this->pWorld = portalWorld;
	this->pBody = &pb->worldIndex->pbIndices;
}

void DefaultObject::reportContact(b2Contact* contact, const b2ContactImpulse* impulse, b2Body* contactingBody, b2Fixture* fix2)
{

}
