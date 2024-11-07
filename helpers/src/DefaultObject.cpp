#include "DefaultObject.h"
#include <Game.h>
#include "GameObjects/ForceField.h"
#include "GameObjects/Catapult.h"
#include "DefaultObjects/Gel.h"

std::vector<DefaultObject*> DefaultObject::defaultObjects;

DefaultObject::DefaultObject(PortalWorld* portalWorld)
{
	this->pWorld = portalWorld;
	funnelMasks.reset();
	newFunnelMasks.reset();
	effectiveFunnel = nullptr;

	defaultObjects.push_back(this);
}

DefaultObject::DefaultObject(PortalWorld* portalWorld, PortalBody* pb)
{
	this->pWorld = portalWorld;
	this->wData = pb->worldIndex;
	funnelMasks.reset();
	newFunnelMasks.reset();
	effectiveFunnel = nullptr;

	defaultObjects.push_back(this);
}

void DefaultObject::recreate(bool mirrored)
{
	int i = wData->pbIndices.size() - 1;
	for (; i >= 0; i--)
	{
		PortalBody* portalBody = wData->pbIndices[i];

		for (auto pair : wData->destroyCallbacks)
		{
			pair.first(portalBody, pair.second);
		}

		delete portalBody;
	}

	PortalBody* portalBody = pWorld->createCloneBody0(wData, initialPos, 0.0f, mirrored);

	portalBody->worldIndex->gravityScale = 1.0f;
	portalBody->worldIndex->destrTime = -20.0f;

	bodyData* bData = (bodyData*)portalBody->body->GetUserData().pointer;
	bData->extraData = (void*)&pData;
}

void DefaultObject::enterForceField(ForceField* forceField)
{
	if (forceField && !forceField->enabled) return;

	auto npBody = wData->pbIndices;

	if (npBody[0]->worldIndex->destrTime > -10.0f)
		return;

	ALuint source = Sounder::object->playSound(ForceField::emancipationBuffer, 0.5);
	b2Vec2 pPos = npBody.at(0)->body->GetPosition();
	Sounder::object->setSoundPosition(source, glm::vec3(pPos.x, pPos.y, 0.0));

	if (TestPlayer::object->holdBody && TestPlayer::object->holdBody->worldIndex == npBody[0]->worldIndex)
	{
		TestPlayer::object->releaseB();
	}

	ForceField::destroyCList.push(npBody[0]->worldIndex);
	npBody[0]->worldIndex->destrTime = Game::object->gameTime - 0.5;
	npBody[0]->worldIndex->gravityScale = 0.0f;

	Game::object->singleTaskRegistry.push_back([npBody]() -> void
	{
		for (PortalBody* pb : npBody)
		{
			b2Vec2 velocity = pb->body->GetLinearVelocity();
			velocity.Normalize();
			velocity *= 0.4;
			pb->body->SetLinearVelocity(velocity);

			float angularVelocity = pb->body->GetAngularVelocity();
			angularVelocity = std::max(std::min(angularVelocity, 1.0f), -1.0f);
			pb->body->SetAngularVelocity(angularVelocity);

			b2MassData massData;
			pb->body->GetMassData(&massData);
			massData.mass = 10000.0;
			massData.I = 10000.0;
			pb->body->SetMassData(&massData);
		}
	});
}

b2Vec2 DefaultObject::tryBounce(b2Contact* contact, b2Fixture* thisFix, b2Fixture* otherFix)
{
	fixtureData* fixData = (fixtureData*)otherFix->GetUserData().pointer;
	b2EdgeShape* edge = (b2EdgeShape*)otherFix->GetShape();
	b2Vec2 edgeNormal;
	if (fixData->type == F_LIGHT_BRIDGE)
	{
		b2Fixture* edge2 = otherFix->GetNext() ? otherFix->GetNext() : otherFix->GetBody()->GetFixtureList();
		b2EdgeShape* edgeShape2 = (b2EdgeShape*)edge2->GetShape();

		edgeNormal = edge->m_vertex1 - edgeShape2->m_vertex1;
	}
	else
	{
		b2Vec2 edgeVec = edge->m_vertex2 - edge->m_vertex1;
		edgeNormal = b2Vec2(edgeVec.y, -edgeVec.x);
	}

	edgeNormal.Normalize();

	b2Vec2 vel = thisFix->GetBody()->GetLinearVelocity();
	float dotRes = b2Dot(-vel, edgeNormal);

	if (dotRes > 3.0f)
	{
		b2WorldManifold wManifold;
		contact->GetWorldManifold(&wManifold);

		if (abs(b2Dot(wManifold.normal, edgeNormal)) < 0.1f)
		{
			return b2Vec2(-1000.0f, -1000.0f);
		}

		b2Vec2 contactPoint = wManifold.points[0];
		if (contact->GetManifold()->pointCount > 1)
		{
			contactPoint += wManifold.points[1];
			contactPoint *= 0.5f;
		}
		bool bounce = Gel::bounceAt(wData->pbIndices[0]->body, otherFix, contactPoint);

		if (bounce)
		{
			float onNormalSpeed = std::max(dotRes, 12.0f);
			b2Vec2 newVelocity;

			if (abs(edgeNormal.x) > 0.1f)
			{
				newVelocity.x = edgeNormal.x * onNormalSpeed;
				newVelocity.y = std::max(vel.y, 6.0f);
			}
			else
			{
				newVelocity.x = vel.x;
				newVelocity.y = onNormalSpeed * edgeNormal.y;
			}

			thisFix->GetBody()->SetLinearVelocity(wData->pbIndices.size() * newVelocity);

			return contactPoint;
		}
	}

	return b2Vec2(-1000.0f, -1000.0f);
}

void DefaultObject::beginContact(b2Contact* contact, b2Fixture* otherFix, b2Fixture* thisFix)
{
	fixtureData* fData = (fixtureData*)otherFix->GetUserData().pointer;
	if (fData)
	{
		switch (fData->type)
		{
		case F_FORCE_FIELD:
		{
			ForceField* forceField = (ForceField*)fData->data;
			if (forceField->enabled)
			{
				enterForceField((ForceField*)fData->data);
			}
		}
		break;
		case F_CATAPULT:
		{
			if (!TestPlayer::object->holdBody || TestPlayer::object->holdBody->worldIndex != wData)
			{
				Catapult* catapult = (Catapult*)fData->data;
				catapult->sendIt(this, wData->pbIndices[0]->body, wData->pbIndices.size());
			}
		}
		break;
		case F_EDGE:
		case F_LIGHT_BRIDGE:
			tryBounce(contact, thisFix, otherFix);
			break;
		default:
			break;
		}
		
	}
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
