#include "GameObjects/ForceField.h"
#include "SwapChain.h"
#include "ThinDrawer.h"
#include "LevelerTestCase.h"
#include "Overlayer.h"

std::vector<ForceField*> ForceField::fields;
ALuint ForceField::emancipationBuffer;
s_perTextureData* ForceField::ptData;
s_texture* ForceField::tex;
std::vector<std::pair<pbWorldData*, float>> ForceField::destroyCList;

static b2Vec2 dirs[4] = { { 0.0, 1.0 }, { -1.0, 0.0 }, { 0.0, -1.0 }, { 1.0, 0.0 } };
static b2Vec2 perpDirs[4] = { { 1.0, 0.0 }, { 0.0, 1.0 }, { 1.0, 0.0 }, { 0.0, 1.0 } };

ForceField::ForceField(PortalWorld* portalWorld, s_objectDataGrid* objData)
{
	fieldRenderOffset = objData->extraOffsets[0];

	b2Vec2 perpDir = perpDirs[objData->rotation % 4];
	b2Vec2 dir = dirs[objData->rotation % 4];

	b2Vec2 midPos = 0.5f * (b2Vec2(objData->pos.x, objData->pos.y) + b2Vec2(objData->extraPos.x, objData->extraPos.y)) + b2Vec2(0.5f, 0.5f);
	float len;
	b2Vec2 size;

	b2PolygonShape pShapes[2];

	const float height = 0.1;
	const float hWidth = 0.4;

	if ((objData->rotation % 4) % 2 == 0)
	{
		size = b2Vec2(0.3f, abs(objData->extraPos.y - objData->pos.y) * 0.5f + 0.5f);
		len = size.y;

		pShapes[0].SetAsBox(hWidth, height, b2Vec2(0.0f, -size.y + height * 0.5), 0.0f);
		pShapes[1].SetAsBox(hWidth, height, b2Vec2(0.0f, +size.y - height * 0.5), 0.0f);
	}
	else
	{
		size = b2Vec2(abs(objData->extraPos.x - objData->pos.x) * 0.5f + 0.5f, 0.3f);
		len = size.x;

		pShapes[0].SetAsBox(height, hWidth, b2Vec2(-size.x + height * 0.5, 0.0f), 0.0f);
		pShapes[1].SetAsBox(height, hWidth, b2Vec2(+size.x - height * 0.5, 0.0f), 0.0f);
	}

	b2BodyDef bDef;
	bDef.type = b2_staticBody;
	bDef.position = midPos;

	body = portalWorld->CreateBody(&bDef);

	body->CreateFixture(&pShapes[0], 0.0f);
	body->CreateFixture(&pShapes[1], 0.0f);

	fixtureData* fData = new fixtureData();
	fData->type = F_FORCE_FIELD;
	fData->data = (void*)this;
	fData->extraData = nullptr;

	b2EdgeShape eShape;
	eShape.SetTwoSided(len * dir + 0.3f * perpDir, -len * dir + 0.3f * perpDir);

	b2FixtureDef fDef;
	fDef.shape = &eShape;
	fDef.isSensor = true;

	b2Fixture* fix = body->CreateFixture(&fDef);
	fix->GetUserData().pointer = (uintptr_t)fData;

	eShape.SetTwoSided(len * dir - 0.3f * perpDir, -len * dir - 0.3f * perpDir);

	fix = body->CreateFixture(&fDef);
	fix->GetUserData().pointer = (uintptr_t)fData;

	fields.push_back(this);

	ThinDrawer* thinDrawer = ThinDrawer::object;

	s_blueFieldUbo* blueUbo;
	vkMapMemory(thinDrawer->logicalDevice, thinDrawer->blueField->uniformBuffers[0].memory,
		fieldRenderOffset * sizeof(s_blueFieldUbo), sizeof(s_blueFieldUbo), 0, (void**)&blueUbo);

	startEnabled = objData->startActive;
	if (!startEnabled)
	{
		blueUbo->data = glm::vec4(0.8, ThinDrawer::object->gameTime, 0.0, ThinDrawer::object->gameTime);
	}
	else
	{
		blueUbo->data = glm::vec4(0.8, ThinDrawer::object->gameTime, 1.0, ThinDrawer::object->gameTime);
	}
	blueUbo->colour.w = 1.0f;

	vkUnmapMemory(thinDrawer->logicalDevice, thinDrawer->blueField->uniformBuffers[0].memory);
}

ForceField::~ForceField()
{
	fixtureData* fData = (fixtureData*)body->GetFixtureList()->GetUserData().pointer;
	body->GetFixtureList()->GetUserData().pointer = 0;
	body->GetFixtureList()->GetUserData().pointer = 0;

	delete fData;
}

void ForceField::enable()
{
	ThinDrawer* thinDrawer = ThinDrawer::object;
	
	fixtureData* fData = (fixtureData*)body->GetFixtureList()->GetUserData().pointer;
	fData->type = F_FORCE_FIELD;

	bool runPlayer = false;
	for (b2ContactEdge* contactEdge = body->GetContactList(); contactEdge != nullptr; contactEdge = contactEdge->next)
	{
		b2Fixture* otherFix = contactEdge->contact->GetFixtureA()->IsSensor() ?
			contactEdge->contact->GetFixtureB() : contactEdge->contact->GetFixtureA();

		bodyData* bData = (bodyData*)otherFix->GetBody()->GetUserData().pointer;
		if (bData && bData->extraData)
		{
			g_PhysicsData* otherData = (g_PhysicsData*)bData->extraData;
			if (otherData)
			{
				if (otherData->type & PLAYER_O && !runPlayer)
				{
					runPlayer = true;
					TestPlayer* tp = (TestPlayer*)otherData->data;
					tp->enterForceField(this);
				}
				else if (otherData->type & DESTRUCTABLES)
				{
					DefaultObject* dObj = (DefaultObject*)otherData->data;
					dObj->enterForceField(this);
				}
			}
		}
	}

	s_blueFieldUbo* blueUbo;
	vkMapMemory(thinDrawer->logicalDevice, thinDrawer->blueField->uniformBuffers[0].memory,
		fieldRenderOffset * sizeof(s_blueFieldUbo), sizeof(s_blueFieldUbo), 0, (void**)&blueUbo);
	blueUbo->data = glm::vec4(0.8, ThinDrawer::object->gameTime, 1.0, ThinDrawer::object->gameTime);
	vkUnmapMemory(thinDrawer->logicalDevice, thinDrawer->blueField->uniformBuffers[0].memory);
}

void ForceField::disable()
{
	ThinDrawer* thinDrawer = ThinDrawer::object;

	s_blueFieldUbo* blueUbo;
	vkMapMemory(thinDrawer->logicalDevice, thinDrawer->blueField->uniformBuffers[0].memory,
		fieldRenderOffset * sizeof(s_blueFieldUbo), sizeof(s_blueFieldUbo), 0, (void**)&blueUbo);
	blueUbo->data = glm::vec4(0.8, ThinDrawer::object->gameTime, 0.0, ThinDrawer::object->gameTime);
	vkUnmapMemory(thinDrawer->logicalDevice, thinDrawer->blueField->uniformBuffers[0].memory);
}

void ForceField::toDeath()
{
	ThinDrawer* thinDrawer = ThinDrawer::object;

	for (ForceField* ff : fields)
	{
		s_blueFieldUbo* blueUbo;
		vkMapMemory(thinDrawer->logicalDevice, thinDrawer->blueField->uniformBuffers[0].memory,
			ff->fieldRenderOffset * sizeof(s_blueFieldUbo), sizeof(s_blueFieldUbo), 0, (void**)&blueUbo);
		blueUbo->data.w = -1.0f;
		vkUnmapMemory(thinDrawer->logicalDevice, thinDrawer->blueField->uniformBuffers[0].memory);
	}
}

void ForceField::toLeveler()
{
	ThinDrawer* thinDrawer = ThinDrawer::object;

	for (ForceField* ff : fields)
	{
		s_blueFieldUbo* blueUbo;
		vkMapMemory(thinDrawer->logicalDevice, thinDrawer->blueField->uniformBuffers[0].memory,
			ff->fieldRenderOffset * sizeof(s_blueFieldUbo), sizeof(s_blueFieldUbo), 0, (void**)&blueUbo);
		blueUbo->data = glm::vec4(0.8f, ThinDrawer::object->gameTime, 1.0f, -1.0f);
		blueUbo->colour.w = ff->startEnabled ? 1.0f : 0.3f;
		vkUnmapMemory(thinDrawer->logicalDevice, thinDrawer->blueField->uniformBuffers[0].memory);

		delete ff;
	}

	fields.clear();
	destroyCList = {};
}

void ForceField::staticInit()
{
	emancipationBuffer = Sounder::object->loadSound("emancipate.wav");

	constexpr int index = getLevelObjectIndex(FORCE_FIELD) - 4;
	ptData = &Leveler::object->perTextureData[index];
	tex = Leveler::object->ol->items[index + 4].tex;

	ThinDrawer::object->drawFuncs.push_back({ render, nullptr });
}

void ForceField::render(VkCommandBuffer cmdBuff, void* ptr)
{
	ManyTexture* mt = Leveler::object->ol->mt;
	ThinDrawer* td = Leveler::object->thinDrawer;

	const VkDeviceSize offsets = 0;

	if (ptData->currentOffset - ptData->offsetStart == 0)
	{
		return;
	}

	vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, mt->pipeline);

	vkCmdBindVertexBuffers(cmdBuff, 0, 1, &td->wallLight->buffers[0].buffer, &offsets);

	vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
		mt->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &td->camOffset);

	vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
		mt->pipelineLayout, 1, 1, &mt->descriptorSets[1], 0, VK_NULL_HANDLE);

	vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
		mt->pipelineLayout, 2, 1, &tex->set, 0, VK_NULL_HANDLE);

	vkCmdDraw(cmdBuff, td->wallLight->buffers[0].count, ptData->currentOffset - ptData->offsetStart, 0, ptData->offsetStart);
}

void ForceField::update()
{
	for (int i = 0; i < destroyCList.size(); i++)
	{	
		auto data = &destroyCList[i];
		auto pbwData = data->first;

		if (!Game::object->isPaused)
		{
			data->second += Game::object->deltaTime;
		}

		pbwData->destrTime = ThinDrawer::object->gameTime - data->second;

		if (data->second >= 2.0f)
		{
			Game::object->pWorld->fullDestroys.push_back(pbwData);

			destroyCList[i] = destroyCList.back();
			destroyCList.pop_back();
			i--;
		}
	}
}