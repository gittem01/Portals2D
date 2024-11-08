#include "GameObjects/Catapult.h"
#include "Overlayer.h"
#include "PolynomialShader.h"
#include "DefaultObjects/Turret.h"
#include "DefaultObject.h"

#define JUMPERS (TURRET_O | PLAYER_O | SPHERE_CUBE_O | LASER_CUBE_O | DEFAULT_CUBE_O)

std::vector<Catapult*> Catapult::catapults;
std::set<DefaultObject*> Catapult::hitObjects;
s_texture* Catapult::headTex;
s_texture* Catapult::bodyTex;
ALuint Catapult::buffer;

static b2Vec2 bodyOffsets[4] =
{
						b2Vec2(0.5f, 0.0f),
						b2Vec2(1.0f, 0.5f),
						b2Vec2(0.5f, 1.0f),
						b2Vec2(0.0f, 0.5f),
};

static b2Vec2 fwdDirs[4] =
{
	b2Vec2(0.0f, 1.0f),
	b2Vec2(-1.0f, 0.0f),
	b2Vec2(0.0f, -1.0f),
	b2Vec2(+1.0f, 0.0f)
};

static b2Vec2 rotationOrigins[4] =
{
	b2Vec2(0.45f, 0.0f),
	b2Vec2(0.45f, 0.0f),
};

Catapult::Catapult(PortalWorld* portalWorld, s_objectDataGrid* objData)
{
	PortalWorld* pWorld = Game::object->pWorld;
	int index = objData->extraOffsets[0];

	s_polynomUbo& prData = ThinDrawer::object->polynomShader->datas[index].data;
	float gravity = abs(pWorld->GetGravity().y);

	if (prData.data.x < -111111111.111f)
	{
		float a = prData.equation.x;
		float b = prData.equation.y;
		float c = prData.equation.z;

		float startX = prData.data.y;
		float xLen = prData.data.z;

		float upX = -b / (2.0f * a);
		float upY = a * upX * upX + b * upX + c;

		b2Vec2 p0 = b2Vec2(objData->pos.x + 0.5f, objData->pos.y + 0.5f);

		float ySpeed;
		float xSpeed;

		float yDiff = upY - p0.y;
		if (yDiff < 0.001f)
		{
			float endY = a * (startX + xLen) * (startX + xLen) + b * (startX + xLen) + c;
			yDiff = p0.y - endY;
			float fallTime = sqrtf(2.0f * yDiff / gravity);
			
			ySpeed = 0.0f;
			xSpeed = xLen / fallTime;
		}
		else
		{
			ySpeed = sqrtf(2.0f * gravity * yDiff);
			if (upX > p0.x && xLen < 0.0f)
			{
				ySpeed *= -1.0f;
			}
			xSpeed = (upX - p0.x) / (ySpeed / gravity);
		}

		sendVelocity = b2Vec2(xSpeed, ySpeed);
	}
	else
	{
		float startY = prData.data.y;
		float yLen = prData.data.z;

		float ySpeed;
		if (yLen < 0.0f)
		{
			ySpeed = std::min(yLen, 10.0f);
		}
		else
		{
			ySpeed = sqrtf(2.0f * gravity * yLen);
		}

		sendVelocity = b2Vec2(0.0f, ySpeed);
	}
	
	sendSpeed = sendVelocity.Length();

	b2BodyDef bDef;
	bDef.type = b2_kinematicBody;
	bDef.position = b2Vec2(objData->pos.x, objData->pos.y) + bodyOffsets[objData->rotation % 4];
	bDef.angle = objData->rotation * 0.5f * b2_pi;

	body = portalWorld->CreateBody(&bDef);
	lastHitTime = -10.0;

	b2EdgeShape shape;
	shape.SetTwoSided(b2Vec2(-0.45f, 0.2f), b2Vec2(0.45f, 0.2f));

	b2Fixture* fix = body->CreateFixture(&shape, 0.0f);
	fix->SetSensor(true);

	fixtureData* fData = new fixtureData();
	fData->type = F_CATAPULT;
	fData->data = (void*)this;
	fData->extraData = nullptr;

	fix->GetUserData().pointer = (uintptr_t)fData;
	
	moveDir = fwdDirs[objData->rotation % 4];
	rotationDir = objData->mirrored ? -1.0f : 1.0f;

	rotationOrigin = rotationOrigins[objData->rotation % 2];
	if (!objData->mirrored)
	{
		rotationOrigin = -rotationOrigins[objData->rotation % 2];
		basePos = bDef.position + 0.45f * fwdDirs[(objData->rotation + 3) % 4];
	}
	else
	{
		rotationOrigin = rotationOrigins[objData->rotation % 2];
		basePos = bDef.position - 0.45f * fwdDirs[(objData->rotation + 3) % 4];
	}

	baseRotation = bDef.angle;

	catapultRenderData.movScale.z = 1.0f;
	catapultRenderData.movScale.w = b2_pi * 0.1f;
	catapultRenderData.posRot.w = objData->mirrored ? -1.0f : 1.0f;

	catapultRenderData.movScale.x = rotationOrigin.x;
	catapultRenderData.movScale.y = rotationOrigin.y;

	catapultRenderData.data.x = 0.0f;
	catapultRenderData.data.y = -20.0f;
	catapultRenderData.data.z = -10.0;

	catapults.push_back(this);
}

void Catapult::sendIt(DefaultObject* dObj, b2Body* body, float mult)
{
	lastHitTime = Game::object->gameTime;

	for (PortalBody* pb : dObj->wData->pbIndices)
	{
		pb->body->SetLinearDamping(0.0f);
	}

	body->SetLinearVelocity(mult * sendVelocity);

	float soundAmount = std::max(std::min(sendSpeed * 0.1f, 5.0f), 0.5f);
	ALuint source = Sounder::object->playSound(Catapult::buffer, soundAmount);
	Sounder::object->setSoundPosition(source, glm::vec3(body->GetPosition().x, body->GetPosition().y, 0.0));

	hitObjects.insert(dObj);
}

void Catapult::toDeath()
{
	hitObjects.clear();
}

void Catapult::toLeveler()
{
	catapults.clear();
	hitObjects.clear();
}

void Catapult::staticInit()
{
	/*bodyTex = IOHelper::loadTexture("CatapultBody.png", VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
	buttonTex = IOHelper::loadTexture("CatapultButton.png", VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);*/

	constexpr int index = getLevelObjectIndex(CATAPULT) - 4;

	bodyTex = Leveler::object->ol->items[index + 4].tex;

	ThinDrawer::object->midDrawFuncs.push_back({ render, nullptr });

	buffer = Sounder::object->loadSound("hop.wav");
}

void Catapult::update()
{
	for (auto iter = hitObjects.begin(); iter != hitObjects.end();)
	{
		DefaultObject* dObj = *iter;
		pbWorldData* wData = dObj->wData;

		if (TestPlayer::object->holdBody && TestPlayer::object->holdBody->worldIndex == wData || dObj->effectiveFunnel)
		{
			for (PortalBody* pb : wData->pbIndices)
			{
				pb->body->SetLinearDamping(wData->baseLinearDamping);
			}

			hitObjects.erase(iter++);
			continue;
		}

		bool erased = false;
		for (PortalBody* pb : wData->pbIndices)
		{
			b2Body* body = pb->body;

			for (b2ContactEdge* ce = body->GetContactList(); ce; ce = ce->next)
			{
				b2Contact* contact = ce->contact;

				if (!contact->IsTouching()) continue;

				b2Fixture* fixA = contact->GetFixtureA();
				b2Fixture* fixB = contact->GetFixtureB();

				if (!fixA->IsSensor() && !fixB->IsSensor())
				{
					for (PortalBody* pb2 : wData->pbIndices)
					{
						pb2->body->SetLinearDamping(wData->baseLinearDamping);
					}

					hitObjects.erase(iter++);
					erased = true;
					break;
				}
			}

			if (erased)
			{
				break;
			}
		}

		if (!erased) ++iter;
	}
}

void Catapult::render(VkCommandBuffer cmdBuff, void* ptr)
{
	ManyTexture* mt = Leveler::object->ol->mt;
	ThinDrawer* td = Leveler::object->thinDrawer;

	const VkDeviceSize offsets = 0;

	vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, td->bodyShader->pipeline);

	vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
		td->bodyShader->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &td->camOffset);

	vkCmdBindVertexBuffers(cmdBuff, 0, 1, &Turret::coverVertexBuffer->buffer, &offsets);

	vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
		td->bodyShader->pipelineLayout, 1, 1, &bodyTex->set, 0, VK_NULL_HANDLE);

	for (Catapult* catapult : catapults)
	{
		catapult->catapultRenderData.posRot.x = catapult->basePos.x + catapult->moveDir.x * b2_pi * 0.05f;
		catapult->catapultRenderData.posRot.y = catapult->basePos.y + catapult->moveDir.y * b2_pi * 0.05f;

		if (catapult->lastHitTime + 0.1 > Game::object->gameTime)
		{
			float diff = (float)(catapult->lastHitTime - Game::object->gameTime);
			catapult->catapultRenderData.posRot.z = catapult->baseRotation + diff * 12.0f * catapult->rotationDir;
		}
		else if (catapult->lastHitTime + 0.7 > Game::object->gameTime)
		{
			float diff = (float)(Game::object->gameTime - catapult->lastHitTime); // 0.1 to 0.7
			diff = (diff - 0.1f) / 0.6f; // 0.0 to 1.0
			catapult->catapultRenderData.posRot.z = catapult->baseRotation + (sqrt(diff) * 1.2f - 1.2f) * catapult->rotationDir;
		}
		else
		{
			catapult->catapultRenderData.posRot.z = catapult->baseRotation;
		}

		vkCmdPushConstants(cmdBuff, td->bodyShader->pipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(s_renderBodyPsh), &catapult->catapultRenderData);

		vkCmdDraw(cmdBuff, 6, 1, 0, 0);
	}
}