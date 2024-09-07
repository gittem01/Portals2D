#include "GameObjects/Funnel.h"
#include "SwapChain.h"
#include "ThinDrawer.h"
#include "LevelerTestCase.h"
#include "Gel.h"
#include "Overlayer.h"

#define INV_FILTER_MASK (F_EDGE | F_GEDGE)

std::vector<Funnel*> Funnel::funnels;
s_perTextureData* Funnel::ptData;
s_texture* Funnel::tex;

static b2Vec2 fwdDirs[4] = { { 0.0, 1.0 }, { -1.0, 0.0 }, { 0.0, -1.0 }, { 1.0, 0.0 } };

static b2Vec2 bodyOffsets[4] =
{
	b2Vec2(1.0f, 0.3f),
	b2Vec2(0.7f, 1.0f),
	b2Vec2(1.0f, 0.7f),
	b2Vec2(0.3f, 1.0f),
};

static b2Vec2 vertices[3] = {
	b2Vec2(0.99f, -0.3f),
	b2Vec2(0.99f, +0.3f),
	b2Vec2(0.00f, -0.05f),
};

void Funnel::ssignal()
{
	signalCount++;
}

void Funnel::usignal()
{
	signalCount--;
}

Funnel::Funnel(PortalWorld* portalWorld, s_objectDataGrid* objData)
{
	signalCount = (int)objData->startActive;
	funnelDir = fwdDirs[objData->rotation % 4];

	b2Vec2 offset = bodyOffsets[objData->rotation % 4];
	funnelPos = b2Vec2(objData->pos.x + offset.x, objData->pos.y + offset.y);

	b2BodyDef bDef;
	bDef.type = b2_staticBody;
	bDef.position = funnelPos;
	bDef.angle = objData->rotation * 0.5f * b2_pi;

	b2Body* body = portalWorld->CreateBody(&bDef);

	for (int i = 0; i < 2; i++)
	{
		b2EdgeShape shape;

		shape.SetTwoSided(vertices[i], vertices[i + 1]);
		body->CreateFixture(&shape, 0.0f);
		
		b2Vec2 mirrV0 = vertices[i]; mirrV0.x = -mirrV0.x;
		b2Vec2 mirrV1 = vertices[i + 1]; mirrV1.x = -mirrV1.x;
		shape.SetTwoSided(mirrV1, mirrV0);
		body->CreateFixture(&shape, 0.0f);
	}

	//fixtureData* fData = new fixtureData();
	//fData->type = F_LASER_EMITTER;
	//fData->data = (void*)this;
	//fData->extraData = nullptr;

	//fix->GetUserData().pointer = (uintptr_t)fData;
}


void Funnel::staticInit()
{
	constexpr int index = getLevelObjectIndex(FUNNEL) - 4;

	ptData = &Leveler::object->perTextureData[index];
	tex = Leveler::object->ol->items[index + 4].tex;

	ThinDrawer::object->midDrawFuncs.push_back({ render, nullptr });
}

void Funnel::render(VkCommandBuffer cmdBuff, void* ptr)
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

void Funnel::update()
{
	for (int i = 0; i < funnels.size(); i++)
	{

	}
}