#include "GameObjects/ForceField.h"
#include "SwapChain.h"
#include "ThinDrawer.h"
#include "LevelerTestCase.h"
#include "Overlayer.h"

std::vector<ForceField*> ForceField::fields;

ForceField::ForceField(PortalWorld* portalWorld, s_objectDataGrid* objData)
{
	fieldRenderOffset = objData->extraOffsets[0];

	b2Vec2 midPos = 0.5f * (b2Vec2(objData->pos.x, objData->pos.y) + b2Vec2(objData->extraPos.x, objData->extraPos.y));
	b2Vec2 size;

	if ((objData->rotation % 4) % 2 == 0)
	{
		midPos.x += 0.5f;
		size = b2Vec2(0.3f, abs(objData->extraPos.y - objData->pos.y));
	}
	else
	{
		midPos.y += 0.5f;
		size = b2Vec2(abs(objData->extraPos.x - objData->pos.x), 0.3f);
	}

	b2BodyDef bDef;
	bDef.type = b2_staticBody;
	bDef.position = midPos;

	b2Body* body = portalWorld->CreateBody(&bDef);

	b2PolygonShape pShape;
	pShape.SetAsBox(size.x, size.y);

	b2FixtureDef fDef;
	fDef.shape = &pShape;
	fDef.isSensor = true;

	b2Fixture* fix = body->CreateFixture(&fDef);

	fixtureData* fData = new fixtureData();
	fData->type = F_FORCE_FIELD;
	fData->data = (void*)this;
	fData->extraData = nullptr;

	fix->GetUserData().pointer = (uintptr_t)fData;

	fields.push_back(this);
}

void ForceField::staticInit()
{
	ThinDrawer::object->midDrawFuncs.push_back({ render, nullptr });
	ThinDrawer::object->memMapFuncs.push_back({ memMapFunc, nullptr });
}

void ForceField::render(VkCommandBuffer cmdBuff, void* ptr)
{
	ManyTexture* mt = Leveler::object->ol->mt;
	ThinDrawer* td = Leveler::object->thinDrawer;

	const VkDeviceSize offsets = 0;

	int i = 3;
	for (auto& data : Leveler::object->perTextureData)
	{
		i++;
		if (data.currentOffset - data.offsetStart == 0)
		{
			continue;
		}

		vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, mt->pipeline);

		vkCmdBindVertexBuffers(cmdBuff, 0, 1, &td->wallLight->buffers[0].buffer, &offsets);

		vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
			mt->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &ThinDrawer::object->camOffset);

		vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
			mt->pipelineLayout, 1, 1, &mt->descriptorSets[1], 0, VK_NULL_HANDLE);

		vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
			mt->pipelineLayout, 2, 1, &Leveler::object->ol->items[i].tex->set, 0, VK_NULL_HANDLE);

		vkCmdDraw(cmdBuff, td->wallLight->buffers[0].count, data.currentOffset - data.offsetStart, 0, data.offsetStart);
	}
}

void ForceField::memMapFunc(void* ptr)
{
	ThinDrawer* td = ThinDrawer::object;

}