#include "DefaultObject.h"
#include <Game.h>
#include "GameObjects/ForceField.h"
#include "GameObjects/Catapult.h"
#include "DefaultObjects/Gel.h"
#include "GameObjects/LightBridge.h"

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

bool DefaultObject::enterForceField(ForceField* forceField)
{
	if (forceField && !forceField->enabled) return false;

	auto npBody = wData->pbIndices;

	if (npBody.size() == 0 || npBody[0]->worldIndex->destrTime > -10.0f)
		return false;

	ALuint source = Sounder::object->playSound(ForceField::emancipationBuffer, 0.5);
	b2Vec2 pPos = npBody.at(0)->body->GetPosition();
	Sounder::object->setSoundPosition(source, glm::vec3(pPos.x, pPos.y, 0.0));

	if (TestPlayer::object->holdBody && TestPlayer::object->holdBody->worldIndex == npBody[0]->worldIndex)
	{
		TestPlayer::object->releaseB();
	}

	ForceField::destroyCList.push_back({ npBody[0]->worldIndex, 0.0f });
	npBody[0]->worldIndex->destrTime = ThinDrawer::object->gameTime - 0.5;
	npBody[0]->worldIndex->gravityScale = 0.0f;

	Game::object->singleTaskRegistry.push_back([this]() -> void
	{
		for (PortalBody* pb : wData->pbIndices)
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

	return true;
}

void DefaultObject::paintedBounce(b2WorldManifold& wManifold, b2Body* body)
{
	if (renderGroup && renderGroup->gelIndex == 0)
	{
		b2Vec2 normal = wManifold.normal;

		// randomizeNormal
		normal.x += (rand() % 1000 - 500) * 0.001f;
		normal.y += (rand() % 1000 - 500) * 0.001f;

		b2Vec2 perpLinearVel = body->GetLinearVelocity();
		perpLinearVel += b2Dot(perpLinearVel, -normal) * normal;

		float randomSpeed = 8.0f + (rand() % 3000) * 0.001f;
		float randomAngularSpeed = body->GetAngularVelocity() + 0.5f + (rand() % 1000) * 0.001f;
		randomAngularSpeed = std::clamp(randomAngularSpeed, -2.0f, 2.0f);
		body->SetLinearVelocity(wData->pbIndices.size() * randomSpeed * b2Vec2(normal.x, normal.y) + perpLinearVel);
		body->SetAngularVelocity(randomAngularSpeed);
	}
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
	else if (fixData->type == F_PORTAL)
	{
		Portal* portal = (Portal*)fixData->data;
		if (portal->connections->size() > 0) return b2Vec2(-1000.0f, -1000.0f);

		if (portal->sideFixtures[0])
		{
			otherFix = portal->sideFixtures[0];
		}
		else if (portal->sideFixtures[1])
		{
			otherFix = portal->sideFixtures[1];
		}
		else
		{
			return b2Vec2(-1000.0f, -1000.0f);
		}

		edgeNormal = portal->dir;
	}
	else
	{
		b2Vec2 edgeVec = edge->m_vertex2 - edge->m_vertex1;
		edgeNormal = b2Vec2(edgeVec.y, -edgeVec.x);
	}

	edgeNormal.Normalize();

	b2Vec2 vel = thisFix->GetBody()->GetLinearVelocity();
	float dotRes = b2Dot(-vel, edgeNormal);

	b2WorldManifold wManifold;
	contact->GetWorldManifold(&wManifold);

	if (dotRes > 3.0f)
	{
		if (abs(b2Dot(wManifold.normal, edgeNormal)) < 0.1f)
		{
			paintedBounce(wManifold, thisFix->GetBody());

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
		else
		{
			paintedBounce(wManifold, thisFix->GetBody());
		}
	}
	else
	{
		paintedBounce(wManifold, thisFix->GetBody());
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
		case F_PORTAL:
			if (!TestPlayer::object->holdBody || TestPlayer::object->holdBody->worldIndex != wData)
			{
				tryBounce(contact, thisFix, otherFix);
			}
			
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
	fixtureData* fData = (fixtureData*)fix2->GetUserData().pointer;
	if (fData && fData->type & (F_EDGE | F_LIGHT_BRIDGE | F_PORTAL))
	{
		b2Vec2 pos = thisBody->GetPosition();

		s_edgeGlobalData* eGlobal = Gel::getEdgeData(fData);

		b2Vec2 v1;
		b2Vec2 dirVec;
		int extraOffset;

		if (fData->type & F_LIGHT_BRIDGE)
		{
			b2EdgeShape* edge = (b2EdgeShape*)fix2->GetShape();
			dirVec = edge->m_vertex2 - edge->m_vertex1;
			dirVec.Normalize();
			v1 = edge->m_vertex1;
			s_edgeLocalData* eLocal = (s_edgeLocalData*)fData->data;
			extraOffset = eLocal->globalRange[0];
		}
		else
		{
			v1 = eGlobal->localPoints[0];
			extraOffset = 0;
			dirVec = eGlobal->dir;
		}

		b2Vec2 diff = pos - fix2->GetBody()->GetWorldPoint(v1);
		int index = (int)std::round(b2Dot(diff, dirVec) * 10.0) + extraOffset;
		index = std::max(std::min(index, (int)eGlobal->unitWalls.size() - 1), 0);

		if (eGlobal->unitWalls[index].gelType == ORANGE && eGlobal->unitWalls[index].gelAmount > 0.5f)
		{
			contact->SetFriction(0.1f);
		}
		else if (!TestPlayer::object->holdBody || TestPlayer::object->holdBody->worldIndex != wData)
		{
			// set friction to fixture defaults
			float friction = b2MixFriction(contact->GetFixtureA()->GetFriction(), contact->GetFixtureB()->GetFriction());
			contact->SetFriction(friction);
		}
	}
}

void DefaultObject::reportContact(b2Contact* contact, const b2ContactImpulse* impulse, b2Body* contactingBody, b2Fixture* fix2)
{
	if (fix2->GetBody()->GetType() != b2_dynamicBody && wData->destrTime < -10.0f && wData->pbIndices.size() == 1)
	{
		b2Manifold* manifold = contact->GetManifold();
		b2WorldManifold wMan;
		contact->GetWorldManifold(&wMan);

		float sep = wMan.separations[0];
		if (manifold->pointCount > 1) sep = (sep + wMan.separations[1]) / 2.0f;

		if (sep < -0.15f)
		{
			fixtureData* fData = (fixtureData*)fix2->GetUserData().pointer;

			bool pass = false;
			if (fData && (fData->type & F_LIGHT_BRIDGE))
			{
				LightBridge* lb = (LightBridge*)fData->extraData;
				if (lb->newlyCreated)
				{
					pass = true;

					PortalBody* contactingwData = (PortalBody*)((bodyData*)contactingBody->GetUserData().pointer)->data;

					if (lb->seperations.find(contactingwData) == lb->seperations.end())
					{
						lb->seperations.insert({ contactingwData, fix2->GetBody() });
					}
				}
			}

			if (fData && (fData->type & F_BRIDGE_CAP))
			{
				pass = true;
			}

			if (!pass)
			{
				enterForceField(nullptr);
			}
		}
	}
}
