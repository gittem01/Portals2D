#include "GameObjects/Geller.h"
#include "SwapChain.h"
#include "ThinDrawer.h"
#include "LevelerTestCase.h"
#include "Gel.h"
#include "Overlayer.h"

std::vector<Geller> Geller::gellers;
s_perTextureData* Geller::ptData[4];
s_texture* Geller::tex[4];

glm::vec2 gelSpeeds[4] = { { 0.0, -1.0 }, { +3.0, 0.5 }, { 0.0, 0.0 }, { -3.0, 0.5 } };
glm::vec2 randVecs[4] = { { 1.0, 0.0 }, { 0.0, 1.0 }, { 0.0, 0.0 }, { 0.0, 1.0 } };

int getGellerIndex(LevelObject obj)
{
	switch (obj)
	{
	case BLUE_GELLER:
		return 0;
	case ORANGE_GELLER:
		return 1;
	case WHITE_GELLER:
		return 2;
	case WATER_GELLER:
		return 3;
	default:
		return 3;
	}
}

Geller::Geller(PortalWorld* portalWorld, s_objectDataGrid* objData)
{
	gellerIndex = getGellerIndex(objData->objectType);
	lastGelCreationTime = -10.0;
	enabled = objData->startActive;
	gelCreationPos = glm::vec2(objData->pos.x + 1.0f, objData->pos.y + 1.0f);
	gellerRotation = objData->rotation;

	b2BodyDef bDef;
	bDef.type = b2_staticBody;
	bDef.position = b2Vec2(gelCreationPos.x, gelCreationPos.y);

	b2Body* body = portalWorld->CreateBody(&bDef);

	b2PolygonShape shape;
	shape.SetAsBox(objData->scale.x / 2.0f, objData->scale.y / 2.0f);

	b2Fixture* fix = body->CreateFixture(&shape, 0.0f);

	fixtureData* fData = new fixtureData();
	fData->type = F_GELLER;
	fData->data = (void*)this;
	fData->extraData = nullptr;

	fix->GetUserData().pointer = (uintptr_t)fData;
}

Geller::~Geller()
{

}

void Geller::createGeller(PortalWorld* portalWorld, s_objectDataGrid* objData)
{
	gellers.push_back(Geller(portalWorld, objData));
}

void Geller::staticInit()
{
	constexpr int index = getLevelObjectIndex(BLUE_GELLER) - 4;

	for (int i = 0; i < 4; i++)
	{
		ptData[i] = &Leveler::object->perTextureData[index + i];
		tex[i] = Leveler::object->ol->items[index + i + 4].tex;
	}

	ThinDrawer::object->midDrawFuncs.push_back({ render, nullptr });
}

void Geller::render(VkCommandBuffer cmdBuff, void* ptr)
{
	ManyTexture* mt = Leveler::object->ol->mt;
	ThinDrawer* td = Leveler::object->thinDrawer;

	const VkDeviceSize offsets = 0;

	for (int i = 0; i < 4; i++)
	{
		if (ptData[i]->currentOffset - ptData[i]->offsetStart == 0)
		{
			continue;
		}

		vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, mt->pipeline);

		vkCmdBindVertexBuffers(cmdBuff, 0, 1, &td->wallLight->buffers[0].buffer, &offsets);

		vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
			mt->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &td->camOffset);

		vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
			mt->pipelineLayout, 1, 1, &mt->descriptorSets[1], 0, VK_NULL_HANDLE);

		vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
			mt->pipelineLayout, 2, 1, &tex[i]->set, 0, VK_NULL_HANDLE);

		vkCmdDraw(cmdBuff, td->wallLight->buffers[0].count, ptData[i]->currentOffset - ptData[i]->offsetStart, 0, ptData[i]->offsetStart);
	}
}

void Geller::update()
{
	if (Gel::gelCount > 256) return;

	for (Geller& g : gellers)
	{
		if (!g.enabled) continue;

		if (Game::object->gameTime - g.lastGelCreationTime > 0.075)
		{
			g.lastGelCreationTime = Game::object->gameTime;
			float randVal = (float)rand() / RAND_MAX;
			float randVal2 = (float)rand() / RAND_MAX - 0.5;
			Gel::generateGel(g.gelCreationPos, gelSpeeds[g.gellerRotation] + randVecs[g.gellerRotation] * randVal2 * 0.5f,
				0.35 + randVal * 0.4, g.gellerIndex);
		}
	}
}