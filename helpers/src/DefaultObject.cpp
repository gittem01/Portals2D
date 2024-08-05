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

void DefaultObject::beginContact(b2Contact* contact, b2Fixture* otherFix)
{

}

void DefaultObject::endContact(b2Contact* contact, b2Fixture* otherFix)
{

}

void DefaultObject::preSolve(b2Contact* contact, b2Body* thisBody, b2Fixture* fix2)
{

}

void DefaultObject::reportContact(b2Contact* contact, const b2ContactImpulse* impulse, b2Body* contactingBody, b2Fixture* fix2)
{

}
