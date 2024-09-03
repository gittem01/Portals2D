#include "GameObjects/HazardField.h"
#include "SwapChain.h"
#include "ThinDrawer.h"
#include "LevelerTestCase.h"
#include "Overlayer.h"

std::vector<HazardField*> HazardField::fields;
ALuint HazardField::hazardBuffer;
s_perTextureData* HazardField::ptData;
s_texture* HazardField::tex;

HazardField::HazardField(PortalWorld* portalWorld, s_objectDataGrid* objData)
{
	signalCount = (int)objData->startActive;

	fieldRenderOffset = objData->extraOffsets[0];

	b2Vec2 midPos = 0.5f * (b2Vec2(objData->pos.x, objData->pos.y) + b2Vec2(objData->extraPos.x, objData->extraPos.y)) + b2Vec2(0.5f, 0.5f);
	b2Vec2 size;

	b2PolygonShape pShapes[2];

	const float height = 0.1;
	const float hWidth = 0.4;

	if ((objData->rotation % 4) % 2 == 0)
	{
		size = b2Vec2(0.3f, abs(objData->extraPos.y - objData->pos.y) * 0.5f + 0.5f);

		pShapes[0].SetAsBox(hWidth, height, b2Vec2(0.0f, -size.y + height * 0.5), 0.0f);
		pShapes[1].SetAsBox(hWidth, height, b2Vec2(0.0f, +size.y - height * 0.5), 0.0f);
	}
	else
	{
		size = b2Vec2(abs(objData->extraPos.x - objData->pos.x) * 0.5f + 0.5f, 0.3f);

		pShapes[0].SetAsBox(height, hWidth, b2Vec2(-size.x + height * 0.5, 0.0f), 0.0f);
		pShapes[1].SetAsBox(height, hWidth, b2Vec2(+size.x - height * 0.5, 0.0f), 0.0f);
	}

	b2BodyDef bDef;
	bDef.type = b2_staticBody;
	bDef.position = midPos;

	body = portalWorld->CreateBody(&bDef);

	b2PolygonShape pShape;
	pShape.SetAsBox(size.x, size.y);

	b2FixtureDef fDef;
	fDef.shape = &pShape;
	fDef.isSensor = true;

	body->CreateFixture(&pShapes[0], 0.0f);
	body->CreateFixture(&pShapes[1], 0.0f);

	b2Fixture* fix = body->CreateFixture(&fDef);
	
	fixtureData* fData = new fixtureData();
	fData->type = F_HAZARD_FIELD;
	fData->data = (void*)this;
	fData->extraData = nullptr;

	fix->GetUserData().pointer = (uintptr_t)fData;

	fields.push_back(this);

	ThinDrawer* thinDrawer = ThinDrawer::object;

	s_hazardUbo* hazardUbo;
	vkMapMemory(thinDrawer->logicalDevice, thinDrawer->hazardShader->uniformBuffers[0].memory,
		fieldRenderOffset * sizeof(s_hazardUbo), sizeof(s_hazardUbo), 0, (void**)&hazardUbo);

	startEnabled = objData->startActive;
	if (!startEnabled)
	{
		hazardUbo->data = glm::vec4(0.8, Game::object->gameTime, 0.0, Game::object->gameTime);
		fData->type = F_OTHER;
	}
	else
	{
		hazardUbo->data = glm::vec4(0.8, Game::object->gameTime, 1.0, Game::object->gameTime);
	}
	hazardUbo->colour.w = 1.0f;

	vkUnmapMemory(thinDrawer->logicalDevice, thinDrawer->hazardShader->uniformBuffers[0].memory);
}

void HazardField::toDeath()
{
	ThinDrawer* thinDrawer = ThinDrawer::object;

	for (HazardField* hf : fields)
	{
		s_hazardUbo* hazardUbo;
		vkMapMemory(thinDrawer->logicalDevice, thinDrawer->hazardShader->uniformBuffers[0].memory,
			hf->fieldRenderOffset * sizeof(s_hazardUbo), sizeof(s_hazardUbo), 0, (void**)&hazardUbo);
		hazardUbo->data.w = -1.0f;
		vkUnmapMemory(thinDrawer->logicalDevice, thinDrawer->hazardShader->uniformBuffers[0].memory);
	}
}

void HazardField::toLeveler()
{
	ThinDrawer* thinDrawer = ThinDrawer::object;

	for (HazardField* hf : fields)
	{
		s_hazardUbo* hazardUbo;
		vkMapMemory(thinDrawer->logicalDevice, thinDrawer->hazardShader->uniformBuffers[0].memory,
			hf->fieldRenderOffset * sizeof(s_hazardUbo), sizeof(s_hazardUbo), 0, (void**)&hazardUbo);
		hazardUbo->data = glm::vec4(0.8f, Game::object->gameTime, 1.0f, -1.0f);
		hazardUbo->colour.w = hf->startEnabled ? 1.0f : 0.3f;
		vkUnmapMemory(thinDrawer->logicalDevice, thinDrawer->hazardShader->uniformBuffers[0].memory);
	}
}

void HazardField::ssignal()
{
	ThinDrawer* thinDrawer = ThinDrawer::object;

	signalCount++;

	if (signalCount == 1)
	{
		fixtureData* fData = (fixtureData*)body->GetFixtureList()->GetUserData().pointer;
		fData->type = F_HAZARD_FIELD;

		for (b2ContactEdge* contactEdge = body->GetContactList(); contactEdge != nullptr; contactEdge = contactEdge->next)
		{
			b2Fixture* otherFix = contactEdge->contact->GetFixtureA()->IsSensor() ?
				contactEdge->contact->GetFixtureB() : contactEdge->contact->GetFixtureA();

			bodyData* bData = (bodyData*)otherFix->GetBody()->GetUserData().pointer;
			if (bData && bData->extraData)
			{
				g_PhysicsData* otherData = (g_PhysicsData*)bData->extraData;
				if (otherData && otherData->type & PLAYER_O)
				{
					TestPlayer* tp = (TestPlayer*)otherData->data;
					tp->health = -1000;
					tp->healthCheck();
				}
			}
		}

		s_hazardUbo* hazardUbo;
		vkMapMemory(thinDrawer->logicalDevice, thinDrawer->hazardShader->uniformBuffers[0].memory,
			fieldRenderOffset * sizeof(s_hazardUbo), sizeof(s_hazardUbo), 0, (void**)&hazardUbo);
		hazardUbo->data = glm::vec4(0.8, Game::object->gameTime, 1.0, Game::object->gameTime);
		vkUnmapMemory(thinDrawer->logicalDevice, thinDrawer->hazardShader->uniformBuffers[0].memory);
	}
}

void HazardField::usignal()
{
	ThinDrawer* thinDrawer = ThinDrawer::object;

	signalCount--;

	if (signalCount == 0)
	{
		fixtureData* fData = (fixtureData*)body->GetFixtureList()->GetUserData().pointer;
		fData->type = F_OTHER;

		s_hazardUbo* hazardUbo;
		vkMapMemory(thinDrawer->logicalDevice, thinDrawer->hazardShader->uniformBuffers[0].memory,
			fieldRenderOffset * sizeof(s_hazardUbo), sizeof(s_hazardUbo), 0, (void**)&hazardUbo);
		hazardUbo->data = glm::vec4(0.8, Game::object->gameTime, 0.0, Game::object->gameTime);
		vkUnmapMemory(thinDrawer->logicalDevice, thinDrawer->hazardShader->uniformBuffers[0].memory);
	}
}

void HazardField::staticInit()
{
	//hazardBuffer = Sounder::object->loadSound("hazard.wav");

	constexpr int index = getLevelObjectIndex(HAZARDER) - 4;
	ptData = &Leveler::object->perTextureData[index];
	tex = Leveler::object->ol->items[index + 4].tex;

	ThinDrawer::object->midDrawFuncs.push_back({ render, nullptr });
}

void HazardField::render(VkCommandBuffer cmdBuff, void* ptr)
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

void HazardField::update()
{
	
}