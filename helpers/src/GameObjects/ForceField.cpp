#include "GameObjects/ForceField.h"
#include "SwapChain.h"
#include "ThinDrawer.h"
#include "LevelerTestCase.h"
#include "Overlayer.h"

std::vector<ForceField*> ForceField::fields;
ALuint ForceField::emancipationBuffer;
s_perTextureData* ForceField::ptData;
s_texture* ForceField::tex;
std::queue<pbWorldData*> ForceField::destroyCList;

ForceField::ForceField(PortalWorld* portalWorld, s_objectDataGrid* objData)
{
	fieldRenderOffset = objData->extraOffsets[0];

	b2Vec2 midPos = 0.5f * (b2Vec2(objData->pos.x, objData->pos.y) + b2Vec2(objData->extraPos.x, objData->extraPos.y));
	b2Vec2 size;

	b2PolygonShape pShapes[2];

	const float height = 0.1;
	const float hWidth = 0.4;

	if ((objData->rotation % 4) % 2 == 0)
	{
		midPos.x += 0.5f;
		size = b2Vec2(0.3f, abs(objData->extraPos.y - objData->pos.y) * 0.5f);

		pShapes[0].SetAsBox(hWidth, height, b2Vec2(0.0f, -size.y + height * 0.5), 0.0f);
		pShapes[1].SetAsBox(hWidth, height, b2Vec2(0.0f, +size.y - height * 0.5 + 1.0), 0.0f);
	}
	else
	{
		midPos.y += 0.5f;
		size = b2Vec2(abs(objData->extraPos.x - objData->pos.x) * 0.5f, 0.3f);
		
		pShapes[0].SetAsBox(height, hWidth, b2Vec2(-size.x + height * 0.5, 0.0f), 0.0f);
		pShapes[1].SetAsBox(height, hWidth, b2Vec2(+size.x - height * 0.5 + 1.0, 0.0f), 0.0f);
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

	body->CreateFixture(&pShapes[0], 0.0f);
	body->CreateFixture(&pShapes[1], 0.0f);

	fixtureData* fData = new fixtureData();
	fData->type = F_FORCE_FIELD;
	fData->data = (void*)this;
	fData->extraData = nullptr;

	fix->GetUserData().pointer = (uintptr_t)fData;

	fields.push_back(this);
}

void ForceField::staticInit()
{
	emancipationBuffer = Sounder::object->loadSound("emancipate.wav");

	constexpr int index = getLevelObjectIndex(FORCE_FIELD) - 4;
	ptData = &Leveler::object->perTextureData[index];
	tex = Leveler::object->ol->items[index + 4].tex;

	ThinDrawer::object->midDrawFuncs.push_back({ render, nullptr });
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
	while (!destroyCList.empty())
	{
		pbWorldData* data = destroyCList.front();
		if (data->destrTime + 2.0f < Game::object->gameTime)
		{
			for (PortalBody* pb : data->pbIndices)
			{
				Game::object->pWorld->destroyBodies.insert(pb);
			}

			destroyCList.pop();
		}
		else
		{
			break;
		}
	}
}