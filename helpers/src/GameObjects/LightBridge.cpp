#include "GameObjects/LightBridge.h"
#include "SwapChain.h"
#include "ThinDrawer.h"
#include "Overlayer.h"

#define LIGHT_FILTER_MASK ~(uint32_t)(F_EDGE | F_GEDGE | F_PORTAL)

std::vector<LightBridge> LightBridge::lightBridges;
int LightBridge::numLightBridges = 0;
s_perTextureData* LightBridge::ptData;
s_texture* LightBridge::tex;
LightRay LightBridge::lightRay;

static b2Vec2 fwdDirs[4] = { { 0.0, 1.0 }, { -1.0, 0.0 }, { 0.0, -1.0 }, { 1.0, 0.0 } };

static b2Vec2 bodyOffsets[4] =
{
	b2Vec2(0.5f, 0.1f),
	b2Vec2(0.9f, 0.5f),
	b2Vec2(0.5f, 0.9f),
	b2Vec2(0.1f, 0.5f),
};

static b2Vec2 vertices[4] = {
	b2Vec2(+0.399f, -0.1f),
	b2Vec2(0.25f, -0.05f),
	b2Vec2(0.16f, 0.28f),
	b2Vec2(0.16f, -0.04f),
};

static int extraIndices[6];

void LightBridge::ssignal()
{
	signalCount++;
}

void LightBridge::usignal()
{
	signalCount--;
}

const float lightThick = 0.3f;

LightBridge::LightBridge(PortalWorld* portalWorld, s_objectDataGrid* objData)
{
	signalCount = (int)objData->startActive;
	lightDir = fwdDirs[objData->rotation % 4];
	extraRenderIndex = -1;

	b2Vec2 rightDir = b2Vec2(lightDir.y, -lightDir.x);

	b2Vec2 offset = bodyOffsets[objData->rotation % 4];
	lightPos = b2Vec2(objData->pos.x + offset.x, objData->pos.y + offset.y);

	b2BodyDef bDef;
	bDef.type = b2_staticBody;
	bDef.position = lightPos;
	bDef.angle = objData->rotation * 0.5f * b2_pi;

	b2Body* body = portalWorld->CreateBody(&bDef);

	b2EdgeShape shape;
	for (int i = 0; i < std::size(vertices) - 1; i++)
	{
		shape.SetTwoSided(vertices[i], vertices[i + 1]);
		body->CreateFixture(&shape, 0.0f);

		b2Vec2 mirrV0 = b2Vec2(-vertices[i].x, vertices[i].y);
		b2Vec2 mirrV1 = b2Vec2(-vertices[i + 1].x, vertices[i + 1].y);

		shape.SetTwoSided(mirrV0, mirrV1);
		body->CreateFixture(&shape, 0.0f);
	}

	b2Vec2 mirrLast = b2Vec2(-vertices[std::size(vertices) - 1].x, vertices[std::size(vertices) - 1].y);
	shape.SetTwoSided(vertices[std::size(vertices) - 1], mirrLast);
	body->CreateFixture(&shape, 0.0f);

	s_bridgeUbo* bridgeUbo;
	vkMapMemory(ThinDrawer::object->logicalDevice, ThinDrawer::object->lightBridge->uniformBuffers[0].memory,
		objData->extraOffsets[0] * sizeof(s_bridgeUbo), sizeof(s_bridgeUbo), 0, (void**)&bridgeUbo);
	memcpy(&data, bridgeUbo, sizeof(s_bridgeUbo));
	vkUnmapMemory(ThinDrawer::object->lightBridge->logicalDevice, ThinDrawer::object->lightBridge->uniformBuffers[0].memory);

	renderOffset = numLightBridges - 1;
	reqMapCount = ThinDrawer::object->swapChain->imageCount;

	maxSend = abs(data.points.x - data.points.z) + abs(data.points.y - data.points.w) + 0.2f;

	lightLasers[0] = lightPos + 0.5f * lightThick * rightDir;
	lightLasers[1] = lightPos - 0.5f * lightThick * rightDir;

	//fixtureData* fData = new fixtureData();
	//fData->type = F_LIGHT_BRIDGE;
	//fData->data = (void*)this;
	//fData->extraData = nullptr;

	//fix->GetUserData().pointer = (uintptr_t)fData;
}

LightBridge* LightBridge::createLightBridge(PortalWorld* portalWorld, s_objectDataGrid* objData)
{
	LightBridge* lbrdg = &lightBridges[numLightBridges++];
	return new (lbrdg) LightBridge(portalWorld, objData);
}

void LightBridge::toDeath()
{
	for (int i = 0; i < 6; i++) extraIndices[i] = 0;
}


void LightBridge::toLeveler()
{
	numLightBridges = 0;

	for (int i = 0; i < 6; i++) extraIndices[i] = 0;
}

void LightBridge::staticInit()
{
	constexpr int index = getLevelObjectIndex(LIGHT_BRIDGE) - 4;

	ptData = &Leveler::object->perTextureData[index];
	tex = Leveler::object->ol->items[index + 4].tex;

	ThinDrawer::object->memMapFuncs.push_back({ memMap, nullptr });
	ThinDrawer::object->midDrawFuncs.push_back({ render, nullptr });

	for (int i = 0; i < 6; i++) extraIndices[i] = 0;
}

void LightBridge::memMap(void* ptr)
{
	int swpNum = ThinDrawer::object->lastSwapChainImageIndex + 1;

	for (int i = 0; i < numLightBridges; i++)
	{
		LightBridge* lb = &lightBridges[i];

		if (lb->reqMapCount > 0)
		{
			s_bridgeUbo* bridgeUbo;
			vkMapMemory(ThinDrawer::object->logicalDevice, ThinDrawer::object->lightBridge->uniformBuffers[0].memory,
				(lb->renderOffset + numLightBridges * swpNum) * sizeof(s_bridgeUbo), sizeof(s_bridgeUbo), 0, (void**)&bridgeUbo);
			lb->data.data.y = lb->totalSize;
			memcpy(bridgeUbo, &lb->data, sizeof(s_bridgeUbo));
			vkUnmapMemory(ThinDrawer::object->lightBridge->logicalDevice, ThinDrawer::object->lightBridge->uniformBuffers[0].memory);

			float offset = lb->lastLen + 0.1f;
			for (int j = 0; j < lb->renderLightPositions.size(); j++)
			{
				s_lightRay lRay = lb->renderLightPositions[j];
				b2Vec2 diff = lRay.p1 - lRay.p0;
				float len = diff.Normalize();

				vkMapMemory(ThinDrawer::object->logicalDevice, ThinDrawer::object->lightBridge->uniformBuffers[0].memory,
					(lb->extraRenderIndex * 5 + j + numLightBridges * 3 + 6 * ThinDrawer::object->lastSwapChainImageIndex) * sizeof(s_bridgeUbo),
					sizeof(s_bridgeUbo), 0, (void**)&bridgeUbo);
				
				bridgeUbo->data.x = lb->data.data.x; bridgeUbo->data.z = lb->data.data.z;
				bridgeUbo->data.y = lb->totalSize;
				bridgeUbo->data.w = offset;
				bridgeUbo->points = glm::vec4(lRay.p0.x, lRay.p0.y, lRay.p1.x, lRay.p1.y);

				vkUnmapMemory(ThinDrawer::object->lightBridge->logicalDevice, ThinDrawer::object->lightBridge->uniformBuffers[0].memory);

				offset += len;
			}

			lb->reqMapCount--;
		}
	}
}

void LightBridge::render(VkCommandBuffer cmdBuff, void* ptr)
{
	ManyTexture* mt = Leveler::object->ol->mt;
	ThinDrawer* td = Leveler::object->thinDrawer;

	const VkDeviceSize offsets = 0;

	if (ptData->currentOffset - ptData->offsetStart == 0)
	{
		return;
	}

	vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, td->lightBridge->pipeline);

	vkCmdBindVertexBuffers(cmdBuff, 0, 1, &td->wallLight->buffers[0].buffer, &offsets);

	vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
		td->lightBridge->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &td->camOffset);

	vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
		td->lightBridge->pipelineLayout, 1, 1, &td->lightBridge->descriptorSets[1], 0, VK_NULL_HANDLE);

	vkCmdDraw(cmdBuff, td->wallLight->buffers[0].count, numLightBridges, 0, numLightBridges * (td->lastSwapChainImageIndex + 1));

	for (int i = 0; i < 6; i++)
	{
		if (extraIndices[i] <= 0) continue;

		vkCmdDraw(cmdBuff, td->wallLight->buffers[0].count, extraIndices[i], 0, numLightBridges * 3 + i * 5 + 6 * td->lastSwapChainImageIndex);
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

Portal* sendRays(float maxLen, b2Vec2 p, b2Vec2 dir, int portalCounter, float* minLenOut)
{
	b2Vec2 rightDir = b2Vec2(dir.y, -dir.x);

	bool allPortal = true;
	float minLen = 10000.0f;
	b2Fixture* lastFix = nullptr;
	for (int i = -1; i < 2; i += 2)
	{
		b2Vec2 p0 = p + i * 0.5f * lightThick * rightDir;
		LightBridge::lightRay.sendRay(p0, dir, maxLen + 0.1f, LIGHT_FILTER_MASK, portalCounter);
		maxLen = LightBridge::lightRay.minFraction * (maxLen + 0.1f);
		minLen = std::min(minLen, maxLen);

		if (LightBridge::lightRay.closestFixture && (lastFix == LightBridge::lightRay.closestFixture || i == -1))
		{
			fixtureData* fData = (fixtureData*)LightBridge::lightRay.closestFixture->GetUserData().pointer;
			allPortal &= fData->type == F_PORTAL;
		}
		else
		{
			allPortal = false;
		}

		lastFix = LightBridge::lightRay.closestFixture;
	}

	*minLenOut = minLen;

	if (allPortal)
	{
		fixtureData* fData = (fixtureData*)LightBridge::lightRay.closestFixture->GetUserData().pointer;
		return (Portal*)fData->data;
	}
	else
	{
		return nullptr;
	}
}

void LightBridge::update()
{
	for (int i = 0; i < numLightBridges; i++)
	{
		LightBridge* lb = &lightBridges[i];

		if (lb->signalCount <= 0) continue;

		b2Vec2 raySendPos = lb->lightPos;
		b2Vec2 rayDir = lb->lightDir;

		lb->totalSize = 0.0f;
		float minLen;
		Portal* portal = sendRays(lb->maxSend, raySendPos, rayDir, 0, &minLen);

		lb->totalSize += minLen + 0.1f;

		if (abs(lb->lastLen - minLen) > 0.001f)
		{
			lb->reqMapCount = ThinDrawer::object->swapChain->imageCount;
			lb->lastLen = minLen;
		}

		lb->lastLen = minLen;

		lb->data.points.z = raySendPos.x + minLen * rayDir.x;
		lb->data.points.w = raySendPos.y + minLen * rayDir.y;

		bool isPortal = portal && portal->connections->size() > 0;

		if (isPortal && lb->extraRenderIndex < 0)
		{
			for (int i = 0; i < 8; i++)
			{
				if (!extraIndices[i])
				{
					lb->extraRenderIndex = i;
					break;
				}
			}
		}
		else if (!isPortal && lb->extraRenderIndex >= 0)
		{
			extraIndices[lb->extraRenderIndex] = 0;
			lb->extraRenderIndex = -1;
		}

		lb->renderLightPositions.clear();

		for (; isPortal;)
		{
			b2Vec2 endPos = raySendPos + minLen * rayDir;

			portalConnection* conn = portal->connections[0][0];
			Portal* otherPortal = conn->portal2;

			b2Vec2 midP = PortalWorld::getRelativePortalPosition_W(endPos, conn);

			raySendPos = midP - 0.2f * otherPortal->dir;
			rayDir = otherPortal->dir;
			portal = sendRays(400.0f, raySendPos, rayDir, 1, &minLen);

			isPortal = portal && portal->connections->size() > 0;

			lb->renderLightPositions.push_back({ midP, raySendPos + minLen * rayDir });

			lb->reqMapCount = ThinDrawer::object->swapChain->imageCount;
			lb->totalSize += minLen - 0.2f;
		}

		if (lb->extraRenderIndex > -1)
		{
			extraIndices[lb->extraRenderIndex] = lb->renderLightPositions.size();
		}
	}
}

void LightRay::sendRay(b2Vec2 rayStart, b2Vec2 dirVec, float rayLength, uint32_t filterMask, int portalCounter)
{
	minFraction = 1.0f;
	rayLen = rayLength;
	closestFixture = nullptr;
	this->filterMask = filterMask;
	this->portalCounter = portalCounter;

	b2Vec2 endPos = rayStart + rayLength * dirVec;

	ThinDrawer::object->registerDebugLine(glm::vec4(rayStart.x, rayStart.y, endPos.x, endPos.y), glm::vec4(0.0f, 0.0f, 1.0f, 1.0f), 0.1f, 0.5f);

	Game::object->pWorld->RayCast(this, rayStart, rayStart + rayLength * dirVec);

	endPos = rayStart + rayLength * minFraction * dirVec;

	ThinDrawer::object->registerDebugLine(glm::vec4(rayStart.x, rayStart.y, endPos.x, endPos.y), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), 0.02f, 0.5f);
}


float LightRay::ReportFixture(b2Fixture* fixture, const b2Vec2& point, const b2Vec2& normal, float fraction)
{
	fixtureData* fData = (fixtureData*)fixture->GetUserData().pointer;
	if (!fData) return minFraction;

	if ((uint32_t)fData->type & filterMask)
	{
		return minFraction;
	}

	if (fData->type == F_PORTAL && portalCounter > 0 && fraction * rayLen < 0.4f)
	{
		portalCounter--;
		return minFraction;
	}

	if (fraction <= minFraction)
	{
		minFraction = fraction;
		closestFixture = fixture;
	}

	return minFraction;
}