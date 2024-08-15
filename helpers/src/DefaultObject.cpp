#include "DefaultObject.h"
#include <Game.h>
#include "GameObjects/ForceField.h"

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
	fixtureData* fData = (fixtureData*)otherFix->GetUserData().pointer;
	if (fData && fData->type == F_FORCE_FIELD)
	{
		auto npBody = *pBody;

		if (npBody[0]->worldIndex->destrTime > -10.0f)
			return;
	
		ALuint source = Sounder::object->playSound(ForceField::emancipationBuffer, 0.5);
		b2Vec2 pPos = npBody.at(0)->body->GetPosition();
		Sounder::object->setSoundPosition(source, glm::vec3(pPos.x, pPos.y, 0.0));

		if (TestPlayer::object->holdBody->worldIndex == npBody[0]->worldIndex)
		{
			TestPlayer::object->releaseB();
		}

		ForceField::destroyCList.push(npBody[0]->worldIndex);
		npBody[0]->worldIndex->destrTime = Game::object->gameTime - 0.5;
		*npBody[0]->gravityScale = 0.0f;

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
