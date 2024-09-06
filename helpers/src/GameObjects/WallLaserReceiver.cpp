#include "GameObjects/WallLaserReceiver.h"
#include "SwapChain.h"
#include "ThinDrawer.h"
#include "LevelerTestCase.h"
#include "Overlayer.h"
#include "GameObjects/LaserEmitter.h"

std::list<WallLaserReceiver*> WallLaserReceiver::disableChecks;
std::list<WallLaserReceiver*> WallLaserReceiver::enableChecks;
s_perTextureData* WallLaserReceiver::ptData;
s_texture* WallLaserReceiver::tex;

static b2Vec2 bodyOffsets[4] =
{
						b2Vec2(0.5f, 0.1f),
						b2Vec2(0.9f, 0.5f),
						b2Vec2(0.5f, 0.9f),
						b2Vec2(0.1f, 0.5f),
};

WallLaserReceiver::WallLaserReceiver(PortalWorld* portalWorld, s_objectDataGrid* objData)
{
	disableTime = -10.0;
	enableTime = -10.0;

	b2Vec2 offset = bodyOffsets[objData->rotation % 4];
	b2Vec2 midPos = b2Vec2(objData->pos.x + offset.x, objData->pos.y + offset.y);

	b2BodyDef bDef;
	bDef.type = b2_staticBody;
	bDef.position = midPos;
	bDef.angle = objData->rotation * 0.5 * b2_pi;

	body = portalWorld->CreateBody(&bDef);

	b2PolygonShape pShape;
	pShape.SetAsBox(0.4f, 0.08f);

	b2FixtureDef fDef;
	fDef.shape = &pShape;

	b2Fixture* fix = body->CreateFixture(&fDef);

	fixtureData* fData = new fixtureData();
	fData->type = F_WALL_RECEIVER;
	fData->data = (void*)this;

	fix->GetUserData().pointer = (uintptr_t)fData;
}

void WallLaserReceiver::enable(LaserEmitter* le)
{
	if (enablers.empty() && enableTime < -1.0)
	{
		enableTime = Game::object->gameTime;
		enableChecks.push_back(this);

		if (disableTime > -1.0)
		{
			for (auto iter = disableChecks.begin(); iter != disableChecks.end(); iter++)
			{
				if ((*iter) == this)
				{
					disableTime = -10.0;
					disableChecks.erase(iter);
					break;
				}
			}
		}
	}

	enablers.insert(le);
}

void WallLaserReceiver::disable(LaserEmitter* le)
{
	enablers.erase(le);

	if (enablers.empty() && disableTime < -1.0)
	{
		disableTime = Game::object->gameTime;
		disableChecks.push_back(this);

		if (enableTime > -1.0)
		{
			for (auto iter = enableChecks.begin(); iter != enableChecks.end(); iter++)
			{
				if ((*iter) == this)
				{
					enableTime = -10.0;
					enableChecks.erase(iter);
					break;
				}
			}
		}
	}
}

void WallLaserReceiver::staticInit()
{
	constexpr int index = getLevelObjectIndex(LASER_REC_WALL) - 4;
	ptData = &Leveler::object->perTextureData[index];
	tex = Leveler::object->ol->items[index + 4].tex;

	ThinDrawer::object->midDrawFuncs.push_back({ render, nullptr });
}

void WallLaserReceiver::update()
{
	auto iter = disableChecks.begin();
	while(iter != disableChecks.end())
	{
		WallLaserReceiver* wlr = *iter;
		if (Game::object->gameTime - wlr->disableTime > 0.04)
		{
			wlr->disableTime = -10.0;
			wlr->Transmitter::disable();
			iter = disableChecks.erase(iter);
		}
		else
		{
			break;
		}
	}

	iter = enableChecks.begin();
	while (iter != enableChecks.end())
	{
		WallLaserReceiver* wlr = *iter;
		if (Game::object->gameTime - wlr->enableTime > 0.04)
		{
			wlr->enableTime = -10.0;
			wlr->Transmitter::enable();
			iter = enableChecks.erase(iter);
		}
		else
		{
			break;
		}
	}
}

void WallLaserReceiver::render(VkCommandBuffer cmdBuff, void* ptr)
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