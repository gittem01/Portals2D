#include "GameObjects/Funnel.h"
#include "SwapChain.h"
#include "ThinDrawer.h"
#include "LevelerTestCase.h"
#include "Gel.h"
#include "Overlayer.h"

#define FUNNEL_FILTER_MASK ~(uint32_t)(F_EDGE | F_GEDGE | F_PORTAL)
#define FUNNEL_FILTER_MASK_NP ~(uint32_t)(F_EDGE | F_GEDGE)

std::vector<DefaultObject*> Funnel::handleObjects;
int Funnel::numHandleObjects = 0;
FunnelRay Funnel::funnelRay;
std::vector<Funnel> Funnel::funnels;
int Funnel::numFunnels = 0;
s_perTextureData* Funnel::ptData;
s_texture* Funnel::tex;
bool Funnel::extraRenderChecker[2];

static b2Vec2 fwdDirs[4] = { { 0.0, 1.0 }, { -1.0, 0.0 }, { 0.0, -1.0 }, { 1.0, 0.0 } };
static float sideMults[4] = { 0.5f, 1.5f, -0.5f, -1.5f };

static const float funnelWidth = 1.6f;
const float rayStepSize = funnelWidth / 3.0f;

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

Funnel::Funnel(PortalWorld* portalWorld, s_objectDataGrid* objData)
{
	funnelDir = fwdDirs[objData->rotation % 4];
	baseAngle = PortalWorld::normalizeAngle(objData->rotation * 0.5f * b2_pi);
	turnAngle = baseAngle;
	lastStartPos2 = b2Vec2(-1000.0f, -1000.0f);
	lastLen = -10.0f;
	lastLen2 = -10.0f;
	extraRenderIndex = -1;

	b2Vec2 offset = bodyOffsets[objData->rotation % 4];
	funnelPos = b2Vec2(objData->pos.x + offset.x, objData->pos.y + offset.y);

	soundData.pos0 = glm::vec2(funnelPos.x, funnelPos.y);

	b2BodyDef bDef;
	bDef.type = b2_staticBody;
	bDef.position = funnelPos;
	bDef.angle = objData->rotation * 0.5f * b2_pi;

	b2Body* body = portalWorld->CreateBody(&bDef);

	fixtureData* fData = new fixtureData();
	fData->type = F_FUNNEL;
	fData->data = (void*)this;
	fData->extraData = nullptr;

	for (int i = 0; i < 2; i++)
	{
		b2EdgeShape shape;

		shape.SetTwoSided(vertices[i], vertices[i + 1]);
		b2Fixture* fix = body->CreateFixture(&shape, 0.0f);
		fix->GetUserData().pointer = (uintptr_t)fData;

		b2Vec2 mirrV0 = vertices[i]; mirrV0.x = -mirrV0.x;
		b2Vec2 mirrV1 = vertices[i + 1]; mirrV1.x = -mirrV1.x;
		shape.SetTwoSided(mirrV1, mirrV0);
		fix = body->CreateFixture(&shape, 0.0f);
		fix->GetUserData().pointer = (uintptr_t)fData;
	}

	b2Vec2 rightDir = fwdDirs[(objData->rotation + 1) % 4];

	rayPositions[0] = funnelPos + 0.5f * rayStepSize * rightDir;
	rayPositions[1] = funnelPos + 1.5f * rayStepSize * rightDir;
	rayPositions[2] = funnelPos - 0.5f * rayStepSize * rightDir;
	rayPositions[3] = funnelPos - 1.5f * rayStepSize * rightDir;

	s_funnelUbo* funnelUbo;
	vkMapMemory(ThinDrawer::object->funnelShader->logicalDevice, ThinDrawer::object->funnelShader->uniformBuffers[0].memory,
		objData->extraOffsets[0] * sizeof(s_funnelUbo), sizeof(s_funnelUbo), 0, (void**)&funnelUbo);
	memcpy(&data, funnelUbo, sizeof(s_funnelUbo));

	vkUnmapMemory(ThinDrawer::object->funnelShader->logicalDevice, ThinDrawer::object->funnelShader->uniformBuffers[0].memory);

	data.colour2 = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
	data2 = data;

	rayLen = glm::length(glm::vec2(data.pointts.x, data.pointts.y) - glm::vec2(data.pointts.z, data.pointts.w)) - 0.2;
	
	int totalFunnels = ThinDrawer::object->funnelShader->numFunnels;
	int numSwapchainImages = ThinDrawer::object->swapChain->imageCount;
	reqMapCount = numSwapchainImages;
	funnelRenderOffset = numFunnels;
	arrIndex = numFunnels;

	for (int i = 0; i < numSwapchainImages; i++)
	{
		vkMapMemory(ThinDrawer::object->funnelShader->logicalDevice, ThinDrawer::object->funnelShader->uniformBuffers[0].memory,
			(totalFunnels + funnelRenderOffset + i * (totalFunnels + 2)) * sizeof(s_funnelUbo), sizeof(s_funnelUbo), 0, (void**)&funnelUbo);
		memcpy(funnelUbo, &data, sizeof(s_funnelUbo));
		vkUnmapMemory(ThinDrawer::object->funnelShader->logicalDevice, ThinDrawer::object->funnelShader->uniformBuffers[0].memory);
	}

	Sounder::object->putStatOnGrid(&soundData, "funnelloop.wav", 1.0f, objData->startActive, true);
}

void Funnel::enable()
{
	if (!enabled2)
	{
		data.colour = glm::vec4(0.0f, 0.3f, 1.0f, 1.0f);
		data2.colour = glm::vec4(0.0f, 0.3f, 1.0f, 1.0f);

		data.colour2 = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
		data2.colour2 = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
	}
	else
	{
		data.colour = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
		data2.colour = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);

		data.colour2 = glm::vec4(1.0f, 0.3f, 0.0f, 1.0f);
		data2.colour2 = glm::vec4(1.0f, 0.3f, 0.0f, 1.0f);
	}

	data.data.y += (ThinDrawer::object->gameTime - data.data2.w) * data.data.w;
	data.data2.w = ThinDrawer::object->gameTime;

	data2.data.y += (ThinDrawer::object->gameTime - data2.data2.w) * data2.data.w;
	data2.data2.w = ThinDrawer::object->gameTime;

	reqMapCount = ThinDrawer::object->swapChain->imageCount;
}

void Funnel::disable()
{
	data.colour.w = 0.0f;
	data2.colour.w = 0.0f;

	data.colour2.w = 0.0f;
	data2.colour2.w = 0.0f;

	reqMapCount = ThinDrawer::object->swapChain->imageCount;
}

void Funnel::enable2()
{
	turnAngle = PortalWorld::normalizeAngle(baseAngle + b2_pi);

	data.data.y += (ThinDrawer::object->gameTime - data.data2.w) * data.data.w;
	data.data2.w = ThinDrawer::object->gameTime;
	data.data2.x = data.data2.z + 5.0f;

	if (extraRenderIndex > -1)
	{
		data.data2.x += data2.data2.z;

		data2.data.y += (ThinDrawer::object->gameTime - data2.data2.w) * data2.data.w;
		data2.data2.w = ThinDrawer::object->gameTime;
		data2.data2.x = data2.data2.z + 5.0f;
	}

	data.data.w = 3.0f;
	data2.data.w = 3.0f;
	data.colour2 = glm::vec4(1.0f, 0.3f, 0.0f, enabled ? 1.0f : 0.0f);
	data2.colour2 = glm::vec4(1.0f, 0.3f, 0.0f, enabled ? 1.0f : 0.0f);

	if (enabled)
	{
		data.colour = glm::vec4(0.0f, 0.3f, 1.0f, 1.0f);
		data2.colour = glm::vec4(0.0f, 0.3f, 1.0f, 1.0f);
	}
	else
	{
		data.colour = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
		data2.colour = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
	}

	reqMapCount = ThinDrawer::object->swapChain->imageCount;
}

void Funnel::disable2()
{
	turnAngle = PortalWorld::normalizeAngle(baseAngle);

	data.data.y += (ThinDrawer::object->gameTime - data.data2.w) * data.data.w;
	data.data2.w = ThinDrawer::object->gameTime;
	data.data2.x = 0.0f;

	data2.data.y += (ThinDrawer::object->gameTime - data2.data2.w) * data2.data.w;
	data2.data2.w = ThinDrawer::object->gameTime;
	data2.data2.x = -data.data2.z;

	data.data.w = -3.0f;
	data2.data.w = -3.0f;

	if (enabled)
	{
		data.colour = glm::vec4(0.0f, 0.3f, 1.0f, 1.0f);
		data2.colour = glm::vec4(0.0f, 0.3f, 1.0f, 1.0f);
	}

	reqMapCount = ThinDrawer::object->swapChain->imageCount;
}

void Funnel::ssignal2()
{
	signalCount2++;

	if (signalCount2 == reqSignalCount2)
	{
		if (disabler2)
		{
			enabled2 = false;
			disable2();
		}
		else
		{
			enabled2 = true;
			enable2();
		}
	}
}

void Funnel::usignal2()
{
	signalCount2--;

	if (signalCount2 < reqSignalCount2)
	{
		if (disabler2)
		{
			enabled2 = true;
			enable2();
		}
		else
		{
			enabled2 = false;
			disable2();
		}
	}
}

void Funnel::toDeath()
{
	extraRenderChecker[0] = false; extraRenderChecker[1] = false;
	handleObjects.clear();
	numHandleObjects = 0;
}

void Funnel::toLeveler()
{
	numFunnels = 0;
	extraRenderChecker[0] = false; extraRenderChecker[1] = false;
	handleObjects.clear();
	numHandleObjects = 0;
}

Funnel* Funnel::createFunnel(PortalWorld* portalWorld, s_objectDataGrid* objData)
{
	Funnel* funnel = &funnels[numFunnels];
	new (funnel) Funnel(portalWorld, objData);

	numFunnels++;

	return funnel;
}

void Funnel::staticInit()
{
	constexpr int index = getLevelObjectIndex(FUNNEL) - 4;

	ptData = &Leveler::object->perTextureData[index];
	tex = Leveler::object->ol->items[index + 4].tex;

	ThinDrawer::object->memMapFuncs.push_back({ memMap, nullptr });
	ThinDrawer::object->lateDrawFuncs.push_back({ render, nullptr });

	for (int i = 0; i < numFunnels; i++)
	{
		Funnel* f = &funnels[i];

		if (f->enabled) f->enable();
		else f->disable();

		if (f->enabled2) f->enable2();
		else f->disable2();
	}
}

void memWrite(Funnel* funnel)
{
	s_funnelUbo* funnelUbo;
	vkMapMemory(ThinDrawer::object->funnelShader->logicalDevice, ThinDrawer::object->funnelShader->uniformBuffers[0].memory,
		(Funnel::numFunnels + funnel->funnelRenderOffset + (Funnel::numFunnels + 2) * ThinDrawer::object->frameIndex) * sizeof(s_funnelUbo),
		sizeof(s_funnelUbo), 0, (void**)&funnelUbo);
	memcpy(funnelUbo, &funnel->data, sizeof(s_funnelUbo));
	vkUnmapMemory(ThinDrawer::object->funnelShader->logicalDevice, ThinDrawer::object->funnelShader->uniformBuffers[0].memory);

	if (funnel->extraRenderIndex >= 0)
	{
		int writeOffset = (Funnel::numFunnels * 2 + (Funnel::numFunnels + 2) * ThinDrawer::object->frameIndex + funnel->extraRenderIndex) * sizeof(s_funnelUbo);

		s_funnelUbo* funnelUbo;
		vkMapMemory(ThinDrawer::object->funnelShader->logicalDevice, ThinDrawer::object->funnelShader->uniformBuffers[0].memory,
			writeOffset, sizeof(s_funnelUbo), 0, (void**)&funnelUbo);
		memcpy(funnelUbo, &funnel->data2, sizeof(s_funnelUbo));
		vkUnmapMemory(ThinDrawer::object->funnelShader->logicalDevice, ThinDrawer::object->funnelShader->uniformBuffers[0].memory);
	}

	funnel->reqMapCount--;
}

void Funnel::memMap(void* ptr)
{
	for (int i = 0; i < numFunnels; i++)
	{
		Funnel* f = &funnels[i];

		if (f->reqMapCount > 0)
		{
			memWrite(f);
		}
	}
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

	vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, td->funnelShader->pipeline);
	vkCmdBindVertexBuffers(cmdBuff, 0, 1, &td->debugCircleShader->buffers[0].buffer, &offsets);

	vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
		td->funnelShader->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &td->camOffset);

	vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
		td->funnelShader->pipelineLayout, 1, 1, &td->funnelShader->descriptorSets[1], 0, VK_NULL_HANDLE);

	int offset = (numFunnels + 2) * td->frameIndex + numFunnels;
	vkCmdDraw(cmdBuff, td->debugCircleShader->buffers[0].count, numFunnels, 0, offset);

	for (int i = 0; i < 2; i++)
	{
		if (extraRenderChecker[i])
		{
			vkCmdDraw(cmdBuff, td->debugCircleShader->buffers[0].count, 1, 0, offset + i + numFunnels);
		}
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

void handleObjs()
{
	for (int i = 0; i < Funnel::numHandleObjects; i++)
	{
		DefaultObject* dobj = Funnel::handleObjects[i];
		if (dobj->newFunnelMasks != dobj->funnelMasks)
		{
			if (dobj->newFunnelMasks.none())
			{
				dobj->effectiveFunnel = nullptr;
				dobj->wData->gravityScale = 1.0f;
				Funnel::handleObjects[i--] = Funnel::handleObjects[--Funnel::numHandleObjects];
			}
			else
			{
				dobj->wData->gravityScale = 0.0f;

				bool getNext = false;
				for (int i = 0; i < Funnel::numFunnels; i++)
				{
					bool funnelGone1 = !dobj->newFunnelMasks[i] && dobj->funnelMasks[i] &&
						dobj->effectiveFunnel && dobj->effectiveFunnel->arrIndex == i && !dobj->inPortalFunnel;
					bool funnelGone2 = !dobj->newFunnelMasks[i + MAX_FUNNELS] && dobj->funnelMasks[i + MAX_FUNNELS] &&
						dobj->effectiveFunnel && dobj->effectiveFunnel->arrIndex == i && dobj->inPortalFunnel;

					if (funnelGone1 || funnelGone2)
					{
						getNext = true;
					}

					if (dobj->newFunnelMasks[i] && !dobj->funnelMasks[i])
					{
						dobj->effectiveFunnel = &Funnel::funnels[i];
						dobj->inPortalFunnel = false;
					}
					else if (dobj->newFunnelMasks[i + MAX_FUNNELS] && !dobj->funnelMasks[i + MAX_FUNNELS])
					{
						dobj->effectiveFunnel = &Funnel::funnels[i];
						dobj->inPortalFunnel = true;
					}
				}

				if (getNext)
				{
					for (int i = 0; i < Funnel::numFunnels; i++)
					{
						if (dobj->newFunnelMasks[i])
						{
							dobj->effectiveFunnel = &Funnel::funnels[i];
							dobj->inPortalFunnel = false;
						}
						else if (dobj->funnelMasks[i + MAX_FUNNELS])
						{
							dobj->effectiveFunnel = &Funnel::funnels[i];
							dobj->inPortalFunnel = true;
						}
					}
				}
			}
		}

		dobj->funnelMasks = dobj->newFunnelMasks;
		dobj->newFunnelMasks.reset();
	}
}

void Funnel::update()
{
	PortalWorld* pWorld = Game::object->pWorld;

	for (int i = 0; i < numFunnels; i++)
	{
		Funnel* funnel = &funnels[i];

		if (!funnel->enabled)
		{
			continue;
		}

		std::bitset<4> portalBits = 0;
		b2Vec2 endPoint1;
		b2Vec2 endPoint2;

		float maxLen = funnel->rayLen;
		float minLen = 10000.0f;
		float minLen2 = 10000.0f;
		b2Fixture* lastFix = nullptr;
		for (int i = 0; i < 4; i++)
		{
			funnelRay.sendRay(funnel->rayPositions[i], funnel->funnelDir, maxLen + 1.0f, FUNNEL_FILTER_MASK);
			maxLen = funnelRay.minFraction * (maxLen + 1.0f);
			minLen = std::min(minLen, maxLen);
			if (funnelRay.closestFixture && (lastFix == funnelRay.closestFixture || i == 0))
			{
				fixtureData* fData = (fixtureData*)funnelRay.closestFixture->GetUserData().pointer;
				portalBits[i] = fData && (uint32_t)fData->type & F_PORTAL && ((Portal*)fData->data)->connections->size() > 0;
			}
			lastFix = funnelRay.closestFixture;
		}

		for (auto pair : funnelRay.objMap)
		{
			if (pair.first < minLen)
			{
				if (pair.second->funnelMasks.none() && pair.second->newFunnelMasks.none())
				{
					handleObjects.resize(numHandleObjects + 1);
					handleObjects[numHandleObjects++] = pair.second;

					b2Body* body = pair.second->wData->pbIndices[0]->body;
					b2Vec2 linVel = body->GetLinearVelocity();
					float len = linVel.Normalize();
					linVel *= std::min(len, 3.0f);
					body->SetLinearVelocity(linVel);
				}

				pair.second->newFunnelMasks[funnel->arrIndex] = true;
			}
		}

		funnelRay.objMap.clear();

		endPoint1 = funnel->funnelPos + minLen * funnel->funnelDir;

		if (portalBits.all())
		{
			fixtureData* fData = (fixtureData*)funnelRay.closestFixture->GetUserData().pointer;
			Portal* portal = (Portal*)fData->data;
			portalConnection* conn = portal->connections[0].front();
			Portal* otherPortal = conn->portal2;

			funnel->turnAngle2 = funnel->turnAngle + otherPortal->dirAngle - portal->dirAngle + b2_pi;
			funnel->turnAngle2 = PortalWorld::normalizeAngle(funnel->turnAngle2);

			funnel->data2.data.x = funnel->data.data.x * (conn->isReversed ? -1.0f : 1.0f);

			b2Vec2 p10 = PortalWorld::getRelativePortalPosition_W(endPoint1, conn);

			funnel->relativeSpeed = b2Dot(portal->body->GetLinearVelocity(), funnel->funnelDir) - b2Dot(otherPortal->body->GetLinearVelocity(), funnel->funnelDir2);
			maxLen = 400.0f;
			funnel->funnelDir2 = otherPortal->dir;
			b2Vec2 perpDir = otherPortal->rotDir;
			for (int i = 0; i < 4; i++)
			{
				b2Vec2 p = p10 + rayStepSize * sideMults[i] * perpDir - 0.5f * otherPortal->dir;
				funnelRay.sendRay(p, otherPortal->dir, maxLen + 0.1f, FUNNEL_FILTER_MASK_NP);
				maxLen = funnelRay.minFraction * (maxLen + 0.1f);
				minLen2 = std::min(minLen2, maxLen);
			}

			for (auto pair : funnelRay.objMap)
			{
				if (pair.first < minLen2)
				{
					if (pair.second->funnelMasks.none() && pair.second->newFunnelMasks.none())
					{
						handleObjects.resize(numHandleObjects + 1);
						handleObjects[numHandleObjects++] = pair.second;

						b2Body* body = pair.second->wData->pbIndices[0]->body;
						b2Vec2 linVel = body->GetLinearVelocity();
						float len = linVel.Normalize();
						linVel *= std::min(len, 3.0f);
						body->SetLinearVelocity(linVel);
					}

					pair.second->newFunnelMasks[funnel->arrIndex + MAX_FUNNELS] = true;
				}
			}

			funnelRay.objMap.clear();

			float posChange = (p10 - funnel->lastStartPos2).LengthSquared();
			funnel->lastStartPos2 = p10;

			if (abs(funnel->lastLen2 - minLen2) > 0.001f || posChange > 0.000001f)
			{
				funnel->data2.pointts.x = p10.x;
				funnel->data2.pointts.y = p10.y;
				funnel->data2.pointts.z = p10.x + minLen2 * otherPortal->dir.x;
				funnel->data2.pointts.w = p10.y + minLen2 * otherPortal->dir.y;

				funnel->data.data2.y = minLen - 0.3f + minLen2;

				funnel->data2.data2.y = minLen2 - 1.0f;
				funnel->data2.data2.z = minLen2 - 0.5;

				if (funnel->extraRenderIndex < 0)
				{
					funnel->extraRenderIndex = extraRenderChecker[0];
					extraRenderChecker[funnel->extraRenderIndex] = true;
					if (funnel->enabled2)
					{
						funnel->data2.data2.x = minLen2;
					}
					else
					{
						funnel->data2.data2.x = -minLen - 3.0f;
					}
				}

				funnel->lastLen2 = minLen2;

				funnel->reqMapCount = ThinDrawer::object->swapChain->imageCount;
			}
		}
		else if (funnel->extraRenderIndex >= 0)
		{
			extraRenderChecker[funnel->extraRenderIndex] = false;
			funnel->extraRenderIndex = -1;
			funnel->data.data2.y = minLen - 0.3f;
			funnel->reqMapCount = ThinDrawer::object->swapChain->imageCount;
			funnel->lastLen2 = 10000.0f;
		}

		float diff = funnel->lastLen - minLen;
		float absDiff = abs(diff);
		if (absDiff > 0.001f)
		{
			if (funnel->lastLen < 1000.0f) funnel->data2.data.y -= diff;

			funnel->data.pointts.z = endPoint1.x;
			funnel->data.pointts.w = endPoint1.y;

			funnel->data.data2.y = minLen - 0.3f + (minLen2 < 1000.0f ? minLen2 : 0.0f);
			funnel->data.data2.z = minLen + 0.2f;

			if (funnel->lastLen < minLen)
			{
				if (funnel->enabled2)
				{
					funnel->data.data2.x = minLen + 3.0f + (minLen2 < 1000.0f ? minLen2 : 0.0f);
				}
				else
				{
					funnel->data.data2.x = 0.0f;
				}
			}

			funnel->lastLen = minLen;
			funnel->reqMapCount = ThinDrawer::object->swapChain->imageCount;
		}

		funnel->data2.data2.x = std::min(funnel->data2.data2.x, 50.0f);
		funnel->data.data2.x = std::min(funnel->data.data2.x, 50.0f);
	}

	handleObjs();
}

float sdSegment(b2Vec2 p, b2Vec2 a, b2Vec2 b)
{
	b2Vec2 pa = p - a, ba = b - a;
	float h = std::clamp(b2Dot(pa, ba) / b2Dot(ba, ba), 0.0f, 1.0f);
	return (pa - h * ba).Length();
}

void Funnel::perTickUpdate()
{
	for (int i = 0; i < numHandleObjects; i++)
	{
		DefaultObject* dobj = handleObjects[i];

		if (dobj->wData->destrTime < -5.0f)
		{
			Funnel* f = dobj->effectiveFunnel;
 			b2Body* body;
			
			int indSize = dobj->wData->pbIndices.size();
			if (indSize == 1)
			{
				body = dobj->wData->pbIndices[0]->body;
			}
			else if (indSize > 1)
			{
				b2Vec2 funnelStart = dobj->inPortalFunnel ? f->lastStartPos2 : f->funnelPos;
				b2Vec2 funnelEnd = dobj->inPortalFunnel ? f->lastStartPos2 + f->lastLen2 * f->funnelDir2 : f->funnelPos + f->lastLen * f->funnelDir;
				
				Portal* portal1 = dobj->wData->pbIndices[0]->bodyMaps[0].connection->portal1;
				Portal* portal2 = dobj->wData->pbIndices[0]->bodyMaps[0].connection->portal2;

				float len1 = sdSegment(portal1->pos, funnelStart, funnelEnd);
				float len2 = sdSegment(portal2->pos, funnelStart, funnelEnd);

				if (len1 < len2)
				{
					body = dobj->wData->pbIndices[0]->body;
				}
				else
				{
					body = dobj->wData->pbIndices[1]->body;
				}
			}
			else
			{
				continue;
			}

			float reqSpeed = f->enabled2 ? -1.5f : 1.5f;
			if (dobj->inPortalFunnel)
			{
				float dist = PortalWorld::getRealDist(f->lastStartPos2,
					f->lastStartPos2 + f->funnelDir2, body->GetPosition());

				b2Vec2 linVel = body->GetLinearVelocity();

				b2Vec2 fwdDiff = linVel - (reqSpeed - f->relativeSpeed) * f->funnelDir2;
				linVel -= indSize * 0.005f * fwdDiff;

				b2Vec2 rightVec = b2Vec2(-f->funnelDir2.y, f->funnelDir2.x);
				linVel += indSize * 0.02f * dist * rightVec;

				body->SetLinearVelocity(linVel);

				float angle = body->GetAngle();
				float diff = PortalWorld::normalizeAngle(f->turnAngle2 - angle);
				body->SetAngularVelocity(indSize * 2.0f * diff);
			}
			else
			{
				float dist = PortalWorld::getRealDist(f->funnelPos,
					f->funnelPos + f->funnelDir, body->GetPosition());

				b2Vec2 linVel = body->GetLinearVelocity();

				b2Vec2 fwdDiff = linVel - reqSpeed * f->funnelDir;
				linVel -= indSize * 0.005f * fwdDiff;

				b2Vec2 rightVec = b2Vec2(-f->funnelDir.y, f->funnelDir.x);
				linVel += indSize * 0.02f * dist * rightVec;

				body->SetLinearVelocity(linVel);

				float angle = body->GetAngle();
				float diff = PortalWorld::normalizeAngle(f->turnAngle - angle);
				body->SetAngularVelocity(2.0f * diff);
			}
		}
	}
}

void FunnelRay::sendRay(b2Vec2 rayStart, b2Vec2 dirVec, float rayLength, uint32_t filterMask)
{
	minFraction = 1.0f;
	rayLen = rayLength;
	closestFixture = nullptr;
    this->filterMask = filterMask;

	b2Vec2 endPos = rayStart + rayLength * dirVec;

	ThinDrawer::object->registerDebugLine(glm::vec4(rayStart.x, rayStart.y, endPos.x, endPos.y), glm::vec4(0.0f, 0.0f, 1.0f, 1.0f), 0.1f, 0.5f);

	Game::object->pWorld->RayCast(this, rayStart, rayStart + rayLength * dirVec);

	endPos = rayStart + rayLength * minFraction * dirVec;

	ThinDrawer::object->registerDebugLine(glm::vec4(rayStart.x, rayStart.y, endPos.x, endPos.y), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), 0.02f, 0.5f);
}

float FunnelRay::ReportFixture(b2Fixture* fixture, const b2Vec2& point, const b2Vec2& normal, float fraction)
{
    fixtureData* fData = (fixtureData*)fixture->GetUserData().pointer;
	if (!fData) return minFraction;
	
	if ((uint32_t)fData->type & filterMask)
    {
		bodyData* bData = (bodyData*)fixture->GetBody()->GetUserData().pointer;
		if (bData && bData->extraData && ((g_PhysicsData*)bData->extraData)->type & DEFAULT_OBJECT_O)
		{
			bool portalSideCheck = false;
			PBFixtureData* pbfData = (PBFixtureData*)fData->data;

			if (pbfData && pbfData->pCollisions.size() > 0)
			{
				for (auto pair : pbfData->pCollisions)
				{
					if (pair.portal->getPointSide(point) != pair.side)
					{
						portalSideCheck = true;
						break;
					}
				}
			}

			if (!portalSideCheck)
			{
				objMap.push_back({ fraction * rayLen, (DefaultObject*)((g_PhysicsData*)bData->extraData)->data });
			}
		}

		return minFraction;
    }

    if (fraction <= minFraction)
    {
        minFraction = fraction;
        closestFixture = fixture;
    }

    return minFraction;
}