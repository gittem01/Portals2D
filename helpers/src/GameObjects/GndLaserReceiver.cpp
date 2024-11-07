#include "GameObjects/GndLaserReceiver.h"
#include "SwapChain.h"
#include "ThinDrawer.h"
#include "LevelerTestCase.h"
#include "Overlayer.h"
#include "GameObjects/LaserEmitter.h"

std::vector<GndLaserReceiver> GndLaserReceiver::gndLaserReceivers(MAX_GND_LASER_RECEIVERS);
int GndLaserReceiver::numGndLaserReceivers = 0;
std::list<GndLaserReceiver*> GndLaserReceiver::disableChecks;
std::list<GndLaserReceiver*> GndLaserReceiver::enableChecks;
s_perTextureData* GndLaserReceiver::ptData;
s_texture* GndLaserReceiver::tex;

static b2Vec2 bodyOffsets[4] =
{
						b2Vec2(0.5f, 0.275f),
						b2Vec2(0.725f, 0.5f),
						b2Vec2(0.5f, 0.725f),
						b2Vec2(0.275f, 0.5f),
};

static b2Vec2 polyPoints[4] =
{
						b2Vec2(+0.29f, -0.275f),
						b2Vec2(+0.15f, +0.275f),
						b2Vec2(-0.15f, +0.275f),
						b2Vec2(-0.29f, -0.275f),
};

GndLaserReceiver::GndLaserReceiver(PortalWorld* portalWorld, s_objectDataGrid* objData)
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

	fixtureData* fData = new fixtureData();
	fData->type = F_GND_RECEIVER;
	fData->data = (void*)this;

	for (int i = 0; i < 3; i++)
	{
		b2EdgeShape eShape;
		eShape.SetTwoSided(polyPoints[i], polyPoints[i + 1]);

		b2FixtureDef fDef;
		fDef.shape = &eShape;

		b2Fixture* fix = body->CreateFixture(&fDef);
		
		fix->GetUserData().pointer = (uintptr_t)fData;
	}

	arrIndex = numGndLaserReceivers++;
}

void GndLaserReceiver::enable(LaserEmitter* le)
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

void GndLaserReceiver::disable(LaserEmitter* le)
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

GndLaserReceiver* GndLaserReceiver::create(PortalWorld* portalWorld, s_objectDataGrid* objData)
{
	GndLaserReceiver* glr = &gndLaserReceivers[numGndLaserReceivers];
	return new (glr) GndLaserReceiver(portalWorld, objData);
}

void GndLaserReceiver::toDeath()
{
	disableChecks.clear();
	enableChecks.clear();

	for (int i = 0; i < numGndLaserReceivers; i++)
	{
		GndLaserReceiver& glr = gndLaserReceivers[i];

		glr.Transmitter::disable();

		glr.disableTime = -10.0;
		glr.enableTime = -10.0;
		glr.enablers.clear();
	}
}

void GndLaserReceiver::staticInit()
{
	constexpr int index = getLevelObjectIndex(LASER_REC_GND) - 4;
	ptData = &Leveler::object->perTextureData[index];
	tex = Leveler::object->ol->items[index + 4].tex;

	ThinDrawer::object->midDrawFuncs.push_back({ render, nullptr });
}

void GndLaserReceiver::update()
{
	auto iter = disableChecks.begin();
	while (iter != disableChecks.end())
	{
		GndLaserReceiver* glr = *iter;
		if (Game::object->gameTime - glr->disableTime > 0.04)
		{
			glr->disableTime = -10.0;
			glr->Transmitter::disable();
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
		GndLaserReceiver* glr = *iter;
		if (Game::object->gameTime - glr->enableTime > 0.04)
		{
			glr->enableTime = -10.0;
			glr->Transmitter::enable();
			iter = enableChecks.erase(iter);
		}
		else
		{
			break;
		}
	}
}

void GndLaserReceiver::render(VkCommandBuffer cmdBuff, void* ptr)
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