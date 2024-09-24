#include "GameObjects/LightBridge.h"
#include "GameObjects/Platform.h"
#include "SwapChain.h"
#include "ThinDrawer.h"
#include "Overlayer.h"

#define LIGHT_FILTER_MASK ~(uint32_t)(F_EDGE | F_GEDGE | F_PORTAL)

std::vector<LightBridge> LightBridge::lightBridges;
std::vector<LightBridge*> LightBridge::perTickChecks;
int LightBridge::numLightBridges = 0;
s_perTextureData* LightBridge::ptData;
s_texture* LightBridge::tex;
LightRay LightBridge::lightRay;

const float lightThick = 0.3f;

static b2Vec2 fwdDirs[4] = { { 0.0, 1.0 }, { -1.0, 0.0 }, { 0.0, -1.0 }, { 1.0, 0.0 } };

static b2Vec2 bodyOffsets[4] =
{
	b2Vec2(0.5f, 0.1f),
	b2Vec2(0.9f, 0.5f),
	b2Vec2(0.5f, 0.9f),
	b2Vec2(0.1f, 0.5f),
};

static b2Vec2 vertices[3] = {
	b2Vec2(+0.399f, -0.1f),
	b2Vec2(0.25f, -0.05f),
	b2Vec2(lightThick * 0.5f, 0.3f),
};

static int extraIndices[6];

constexpr int nextExtraGel = 2 * 6 * 4000;
static bool requireDestroy = false;
static int extraWallStartOffset;

LightBridge::LightBridge(PortalWorld* portalWorld, s_objectDataGrid* objData)
{
	lightDir = fwdDirs[objData->rotation % 4];
	extraRenderIndex = -1;

	b2Vec2 rightDir = b2Vec2(lightDir.y, -lightDir.x);

	b2Vec2 offset = bodyOffsets[objData->rotation % 4];
	lightPos = b2Vec2(objData->pos.x + offset.x, objData->pos.y + offset.y);

	b2BodyDef bDef;
	bDef.type = b2_staticBody;
	bDef.position = lightPos;
	bDef.angle = objData->rotation * 0.5f * b2_pi;

	bridgeBody = portalWorld->CreateBody(&bDef);

	b2EdgeShape shape;
	for (int i = 0; i < std::size(vertices) - 1; i++)
	{
		shape.SetTwoSided(vertices[i], vertices[i + 1]);
		bridgeBody->CreateFixture(&shape, 0.0f);

		b2Vec2 mirrV0 = b2Vec2(-vertices[i].x, vertices[i].y);
		b2Vec2 mirrV1 = b2Vec2(-vertices[i + 1].x, vertices[i + 1].y);

		shape.SetTwoSided(mirrV0, mirrV1);
		bridgeBody->CreateFixture(&shape, 0.0f);
	}

	bDef.angle += -b2_pi * 0.5f;
	bDef.type = b2_kinematicBody;
	capBody = portalWorld->CreateBody(&bDef);

	b2Vec2 p1 = b2Vec2(-0.3f, lightThick * 0.5f);
	b2Vec2 p2 = b2Vec2(-0.01f, p1.y);

	shape.SetTwoSided(p1, p2);
	capBody->CreateFixture(&shape, 0.0f);

	p1 = b2Vec2(p1.x, -p1.y);
	p2 = b2Vec2(p2.x, -p2.y);
	shape.SetTwoSided(p1, p2);
	capBody->CreateFixture(&shape, 0.0f);

	p1 = b2Vec2(p2.x, -p2.y);
	shape.SetTwoSided(p2, p1);
	capBody->CreateFixture(&shape, 0.0f);

	s_bridgeUbo* bridgeUbo;
	vkMapMemory(ThinDrawer::object->logicalDevice, ThinDrawer::object->lightBridge->uniformBuffers[0].memory,
		objData->extraOffsets[0] * sizeof(s_bridgeUbo), sizeof(s_bridgeUbo), 0, (void**)&bridgeUbo);
	memcpy(&data, bridgeUbo, sizeof(s_bridgeUbo));
	vkUnmapMemory(ThinDrawer::object->lightBridge->logicalDevice, ThinDrawer::object->lightBridge->uniformBuffers[0].memory);

	data.data.z = 1.0f;
	if (!objData->startActive) data.data.x = 0.0f;

	renderOffset = numLightBridges - 1;
	reqMapCount = ThinDrawer::object->swapChain->imageCount;

	maxSend = abs(data.points.x - data.points.z) + abs(data.points.y - data.points.w) + 0.2f;

	lightLasers[0] = lightPos + 0.5f * lightThick * rightDir;
	lightLasers[1] = lightPos - 0.5f * lightThick * rightDir;

	int wallGelSize = (int)(maxSend * 10.0f);
	
	wallRenderIndex = ThinDrawer::object->wallGel->numWallLines + MAX_PLATFORMS * 4 * ThinDrawer::object->swapChain->imageCount + numLightBridges - 1;
	wallGelStartOffset1 = ThinDrawer::object->wallGel->numWallGels;
	wallGelStartOffset2 = ThinDrawer::object->wallGel->numWallGels + wallGelSize;

	effectiveWallIndex = wallRenderIndex;
	effectiveGelOffset1 = wallGelStartOffset1;
	effectiveGelOffset2 = wallGelStartOffset2;

	ThinDrawer::object->wallGel->numWallGels += 2 * wallGelSize;

	for (int i = 0; i < 2; i++)
	{
		eDatas[i].sideEdges[0] = nullptr; eDatas[i].sideEdges[1] = nullptr;
		eDatas[i].dir = b2Vec2(1.0f, 0.0f);
		eDatas[i].localPoints[0] = b2Vec2(-1.0f, 0.0f);
		eDatas[i].localPoints[1] = b2Vec2(1.0f, 0.0f);
		eDatas[i].perpDir = b2Vec2(0.0f, 1.0f);
		eDatas[i].renderOffset = wallGelStartOffset1 + i * wallGelSize;
		eDatas[i].unitWalls.resize(wallGelSize);
	}

	//fixtureData* fData = new fixtureData();
	//fData->type = F_LIGHT_BRIDGE;
	//fData->data = (void*)this;
	//fData->extraData = nullptr;

	//fix->GetUserData().pointer = (uintptr_t)fData;
}

void LightBridge::enable()
{
	data.data.x = 0.4f;
	reqMapCount = ThinDrawer::object->swapChain->imageCount;
}

void LightBridge::disable()
{
	data.data.x = 0.0f;
	reqMapCount = ThinDrawer::object->swapChain->imageCount;

	if (extraRenderIndex >= 0)
	{
		extraIndices[extraRenderIndex] = 0;
		extraRenderIndex = -1;
	}

	lastLen = -0.1f;

	capBody->SetTransform(lightPos, -b2_pi * 0.5f);

	for (int i = 0; i < bodies.size(); i++)
	{
		b2Fixture* fix = bodies[i]->GetFixtureList();
		fixtureData* fData = (fixtureData*)(fix->GetUserData().pointer - sizeof(fixtureData));
		free(fData);
		fix->GetUserData().pointer = 0; fix->GetNext()->GetUserData().pointer = 0;
		Game::object->pWorld->DestroyBody(bodies[i]);
	}
	bodies.clear();

	eDatas[0].unitWalls.clear();
	eDatas[1].unitWalls.clear();

	renderLightPositions.clear();
}

LightBridge* LightBridge::createLightBridge(PortalWorld* portalWorld, s_objectDataGrid* objData)
{
	LightBridge* lbrdg = &lightBridges[numLightBridges++];
	return new (lbrdg) LightBridge(portalWorld, objData);
}

void LightBridge::toDeath()
{
	for (int i = 0; i < 6; i++) extraIndices[i] = 0;
	perTickChecks.clear();
}


void LightBridge::toLeveler()
{
	numLightBridges = 0;

	for (int i = 0; i < 6; i++) extraIndices[i] = 0;
	perTickChecks.clear();
}

void portalDestrCallback(Portal* portal)
{
	requireDestroy = true;
}

void LightBridge::staticInit()
{
	constexpr int index = getLevelObjectIndex(LIGHT_BRIDGE) - 4;

	ptData = &Leveler::object->perTextureData[index];
	tex = Leveler::object->ol->items[index + 4].tex;

	ThinDrawer::object->memMapFuncs.push_back({ memMap, nullptr });
	ThinDrawer::object->midDrawFuncs.push_back({ render, nullptr });

	Game::object->pWorld->portalDestroyCallbacks.push_back(portalDestrCallback);

	for (int i = 0; i < 6; i++) extraIndices[i] = 0;

	extraWallStartOffset = ThinDrawer::object->wallGel->numWallLines + (numLightBridges * 2 + MAX_PLATFORMS * 4) * ThinDrawer::object->swapChain->imageCount;
}

void LightBridge::memMap(void* ptr)
{
	int swpNum = ThinDrawer::object->lastSwapChainImageIndex + 1;

	for (int i = 0; i < numLightBridges; i++)
	{
		LightBridge* lb = &lightBridges[i];

		if (lb->reqMapCount <= 0) continue;

		s_bridgeUbo* bridgeUbo;
		vkMapMemory(ThinDrawer::object->logicalDevice, ThinDrawer::object->lightBridge->uniformBuffers[0].memory,
			(lb->renderOffset + numLightBridges * swpNum) * sizeof(s_bridgeUbo), sizeof(s_bridgeUbo), 0, (void**)&bridgeUbo);
		lb->data.data.y = lb->totalSize;
		memcpy(bridgeUbo, &lb->data, sizeof(s_bridgeUbo));
		vkUnmapMemory(ThinDrawer::object->lightBridge->logicalDevice, ThinDrawer::object->lightBridge->uniformBuffers[0].memory);
				
		s_wallGelData* wallGel;
		vkMapMemory(ThinDrawer::object->logicalDevice, ThinDrawer::object->wallGel->uniformBuffers[0].memory,
			lb->wallRenderIndex * sizeof(s_wallGelData), sizeof(s_wallGelData), 0, (void**)&wallGel);

		b2Vec2 rightDir = b2Vec2(-lb->lightDir.y, lb->lightDir.x);
		b2Vec2 p0 = lb->lightPos + 0.2f * rightDir;
		b2Vec2 endPos = p0 + (lb->lastLen + 0.1f) * lb->lightDir;

		wallGel->positions = glm::vec4(p0.x, p0.y, endPos.x, endPos.y);
		wallGel->data.x = lb->eDatas[0].unitWalls.size();
		wallGel->data.y = -lb->effectiveGelOffset1;
			
		wallGel->data.z = -1071;
		wallGel->data.w = lb->effectiveGelOffset2;

		vkUnmapMemory(ThinDrawer::object->logicalDevice, ThinDrawer::object->wallGel->uniformBuffers[0].memory);
		
		float offset = lb->lastLen + 0.1f;

		if (lb->renderLightPositions.size() == 0 || lb->extraRenderIndex < 0) goto end;

		vkMapMemory(ThinDrawer::object->logicalDevice, ThinDrawer::object->lightBridge->uniformBuffers[0].memory,
			(lb->extraRenderIndex * 5 + numLightBridges * 3 + 5 * 6 * (swpNum - 1)) * sizeof(s_bridgeUbo),
			sizeof(s_bridgeUbo) * lb->renderLightPositions.size(), 0, (void**)&bridgeUbo);

		vkMapMemory(ThinDrawer::object->logicalDevice, ThinDrawer::object->wallGel->uniformBuffers[0].memory,
			(lb->effectiveWallIndex + (swpNum - 1) * 30) * sizeof(s_wallGelData),
			sizeof(s_wallGelData) * lb->renderLightPositions.size(), 0, (void**)&wallGel);

		for (int j = 0; j < lb->renderLightPositions.size(); j++)
		{
			bool reverseCond = lb->reversed && (j % 2 == 0);
			float reverser = reverseCond ? -1.0f : 1.0f;

			s_lightRay lRay = lb->renderLightPositions[j];
			b2Vec2 diff = lRay.p1 - lRay.p0;
			float len = diff.Normalize();
				
			bridgeUbo[j].data.x = lb->data.data.x * reverser * -1.0f; bridgeUbo[j].data.z = lb->data.data.z;
			bridgeUbo[j].data.y = lb->totalSize;
			bridgeUbo[j].data.w = offset;
			bridgeUbo[j].points = glm::vec4(lRay.p0.x, lRay.p0.y, lRay.p1.x, lRay.p1.y);

			float fVal = (offset - 0.1f) * 10.0f;
			int startOffset = std::floor(fVal);
			float fract = fVal - startOffset;

			b2Vec2 startPos = lRay.p0 - 0.1f * fract * diff - reverser * 0.2f * lb->frames.p2v2;
			b2Vec2 endPos = startPos + (len + 0.2f) * diff;

			wallGel[j].data.x = lb->eDatas[0].unitWalls.size();
			wallGel[j].data.y = (lb->effectiveGelOffset1 + startOffset) * (reverseCond ? -1 : 1);
			wallGel[j].data.z = -1071;
			wallGel[j].data.w = lb->effectiveGelOffset2 + startOffset;
			wallGel[j].positions = glm::vec4(startPos.x, startPos.y, endPos.x, endPos.y);

			offset += len;
		}

		vkUnmapMemory(ThinDrawer::object->logicalDevice, ThinDrawer::object->lightBridge->uniformBuffers[0].memory);

		vkUnmapMemory(ThinDrawer::object->logicalDevice, ThinDrawer::object->wallGel->uniformBuffers[0].memory);

	end:
		lb->reqMapCount--;
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

		vkCmdDraw(cmdBuff, td->wallLight->buffers[0].count, extraIndices[i], 0, numLightBridges * 3 + i * 5 + 5 * 6 * td->lastSwapChainImageIndex);
	}

	// gels
	vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, ThinDrawer::object->wallGel->pipeline);

	vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
		ThinDrawer::object->wallGel->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &ThinDrawer::object->camOffset);

	vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
		ThinDrawer::object->wallGel->pipelineLayout, 1, 1, &ThinDrawer::object->wallGel->descriptorSets[1], 0, nullptr);

	vkCmdDraw(cmdBuff, 6, numLightBridges, 0, ThinDrawer::object->wallGel->numWallLines +
		MAX_PLATFORMS * 4 * td->swapChain->imageCount);

	for (int i = 0; i < 6; i++)
	{
		if (extraIndices[i] <= 0) continue;

		vkCmdDraw(cmdBuff, 6, extraIndices[i], 0, extraWallStartOffset + i * 10 + td->lastSwapChainImageIndex * 30);
	}

	// tex
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
	if (requireDestroy)
	{
		for (int i = 0; i < LightBridge::perTickChecks.size(); i++)
		{
			LightBridge* lb = LightBridge::perTickChecks[i];

			for (int j = 1; j < lb->bodies.size(); j++)
			{
				b2Fixture* fix = lb->bodies[j]->GetFixtureList();
				fixtureData* fData = (fixtureData*)(fix->GetUserData().pointer - sizeof(fixtureData));
				free(fData);
				fix->GetUserData().pointer = 0; fix->GetNext()->GetUserData().pointer = 0;
				Game::object->pWorld->DestroyBody(lb->bodies[j]);
			}

			lb->bodies.resize(std::min((int)lb->bodies.size(), 1));
		}

		requireDestroy = false;
	}

	perTickChecks.clear();

	for (int i = 0; i < numLightBridges; i++)
	{
		LightBridge* lb = &lightBridges[i];

		if (!lb->enabled) continue;

		b2Vec2 raySendPos = lb->lightPos;
		b2Vec2 rayDir = lb->lightDir;

		lb->totalSize = 0.0f;
		float minLen;
		Portal* portal = sendRays(lb->maxSend, raySendPos, rayDir, 0, &minLen);

		b2BodyType bType;
		if (lightRay.closestFixture)
		{
			lb->endBody = lightRay.closestFixture->GetBody();
			bType = lb->endBody->GetType();
		}
		else
		{
			lb->endBody = nullptr;
			bType = b2_staticBody;
		}
		
		bool isPortal = portal && portal->connections->size() > 0;

		if (lb->bodies.size() == 0)
		{
			fixtureData* fData = (fixtureData*)malloc(sizeof(fixtureData) * 2 + sizeof(s_edgeLocalData) * 2);
			s_edgeLocalData* eData = (s_edgeLocalData*)&fData[2];

			fData[0].extraData = nullptr;
			fData[1].extraData = nullptr;

			eData[0].prevNextFixtures[0] = nullptr; eData[0].prevNextFixtures[1] = nullptr;
			eData[1].prevNextFixtures[0] = nullptr; eData[1].prevNextFixtures[1] = nullptr;

			fData[0].data = (void*)&eData[0];
			fData[0].type = F_LIGHT_BRIDGE;

			fData[1].data = (void*)&eData[1];
			fData[1].type = F_LIGHT_BRIDGE;

			eData[0].globalData = &lb->eDatas[0];
			eData[1].globalData = &lb->eDatas[1];

			eData[0].globalRange[0] = 0;
			eData[0].globalRange[1] = std::ceil((minLen + 0.1f) * 10.0f);

			eData[1].globalRange[0] = 0;
			eData[1].globalRange[1] = eData[0].globalRange[1];

			float extraLen = isPortal ? 0.15f : -0.15f;

			b2Vec2 rightDir = b2Vec2(-rayDir.y, rayDir.x);
			
			b2Vec2 start1 = raySendPos + 0.5f * lightThick * rightDir;
			b2Vec2 start2 = raySendPos - 0.5f * lightThick * rightDir;

			b2Vec2 end1 = start1 + (minLen + extraLen) * rayDir;
			b2Vec2 end2 = start2 + (minLen + extraLen) * rayDir;

			b2BodyDef bDef;
			bDef.type = b2_staticBody;
			b2Body* body = Game::object->pWorld->CreateBody(&bDef);

			b2EdgeShape shape;
			shape.SetTwoSided(start1, end1);
			b2Fixture* fix = body->CreateFixture(&shape, 0.0f);
			fix->GetUserData().pointer = (uintptr_t)&fData[0];

			shape.SetTwoSided(start2, end2);
			fix = body->CreateFixture(&shape, 0.0f);
			fix->GetUserData().pointer = (uintptr_t)&fData[1];

			lb->bodies.push_back(body);
		}

		lb->totalSize += minLen + 0.1f;

		if (abs(lb->lastLen - minLen) > 0.001f)
		{
			lb->reqMapCount = ThinDrawer::object->swapChain->imageCount;
			lb->lastLen = minLen;

			b2EdgeShape* eShape = (b2EdgeShape*)lb->bodies[0]->GetFixtureList()->GetShape();
			b2Vec2 p0 = eShape->m_vertex1;
			b2Vec2 p1 = eShape->m_vertex2;
			float len = (p1 - p0).Length();

			float extraLen = isPortal ? 0.15f : -0.15f;

			if (abs(len - extraLen - minLen) > 0.1f)
			{
				b2Body* body = lb->bodies[0];

				fixtureData* fData = (fixtureData*)(body->GetFixtureList()->GetUserData().pointer - sizeof(fixtureData));
				s_edgeLocalData* eData = (s_edgeLocalData*)fData->data;

				eData[0].globalRange[1] = std::ceil((minLen + 0.1f) * 10.0f);
				eData[1].globalRange[1] = eData[0].globalRange[1];

				body->DestroyFixture(body->GetFixtureList());
				body->DestroyFixture(body->GetFixtureList());

				b2Vec2 rightDir = b2Vec2(-rayDir.y, rayDir.x);

				b2Vec2 start1 = raySendPos + 0.5f * lightThick * rightDir;
				b2Vec2 start2 = raySendPos - 0.5f * lightThick * rightDir;

				b2Vec2 end1 = start1 + (minLen + extraLen) * rayDir;
				b2Vec2 end2 = start2 + (minLen + extraLen) * rayDir;

				b2EdgeShape shape;
				shape.SetTwoSided(start1, end1);
				b2Fixture* fix = body->CreateFixture(&shape, 0.0f);
				fix->GetUserData().pointer = (uintptr_t)&fData[0];

				shape.SetTwoSided(start2, end2);
				fix = body->CreateFixture(&shape, 0.0f);
				fix->GetUserData().pointer = (uintptr_t)&fData[1];
			}
		}

		lb->lastLen = minLen;

		lb->data.points.z = raySendPos.x + minLen * rayDir.x;
		lb->data.points.w = raySendPos.y + minLen * rayDir.y;

		if (isPortal && lb->extraRenderIndex < 0)
		{
			for (int i = 0; i < 6; i++)
			{
				if (!extraIndices[i])
				{
					lb->extraRenderIndex = i;

					lb->effectiveWallIndex = extraWallStartOffset + i * 2 * 5;
					lb->effectiveGelOffset1 = ThinDrawer::object->wallGel->numWallGels + i * nextExtraGel;
					lb->effectiveGelOffset2 = lb->effectiveGelOffset1 + nextExtraGel / 2;

					lb->eDatas[0].renderOffset = lb->effectiveGelOffset1;
					lb->eDatas[1].renderOffset = lb->effectiveGelOffset2;

					s_wallPerGelData* gelData;
					vkMapMemory(ThinDrawer::object->logicalDevice, ThinDrawer::object->wallGel->uniformBuffers[1].memory,
						lb->effectiveGelOffset1 * sizeof(s_wallPerGelData), sizeof(s_wallPerGelData) * lb->eDatas[0].unitWalls.size(), 0, (void**)&gelData);
					for (int i = 0; i < lb->eDatas[0].unitWalls.size(); i++)
					{
						gelData[i].data.x = gelTypeToIndex(lb->eDatas[0].unitWalls[i].gelType);
						gelData[i].data.y = lb->eDatas[0].unitWalls[i].gelAmount;
					}
					vkUnmapMemory(ThinDrawer::object->logicalDevice, ThinDrawer::object->wallGel->uniformBuffers[1].memory);

					vkMapMemory(ThinDrawer::object->logicalDevice, ThinDrawer::object->wallGel->uniformBuffers[1].memory,
						lb->effectiveGelOffset2 * sizeof(s_wallPerGelData), sizeof(s_wallPerGelData)* lb->eDatas[1].unitWalls.size(), 0, (void**)&gelData);
					for (int i = 0; i < lb->eDatas[0].unitWalls.size(); i++)
					{
						gelData[i].data.x = gelTypeToIndex(lb->eDatas[1].unitWalls[i].gelType);
						gelData[i].data.y = lb->eDatas[1].unitWalls[i].gelAmount;
					}
					vkUnmapMemory(ThinDrawer::object->logicalDevice, ThinDrawer::object->wallGel->uniformBuffers[1].memory);

					break;
				}
			}
		}
		else if (!isPortal && lb->extraRenderIndex >= 0)
		{
			extraIndices[lb->extraRenderIndex] = 0;
			lb->extraRenderIndex = -1;

			lb->effectiveWallIndex = lb->wallRenderIndex;
			lb->effectiveGelOffset1 = lb->wallGelStartOffset1;
			lb->effectiveGelOffset2 = lb->wallGelStartOffset2;

			lb->eDatas[0].renderOffset = lb->effectiveGelOffset1;
			lb->eDatas[1].renderOffset = lb->effectiveGelOffset2;

			int size = std::ceil(lb->totalSize * 10.0f);
			s_wallPerGelData* gelData;
			vkMapMemory(ThinDrawer::object->logicalDevice, ThinDrawer::object->wallGel->uniformBuffers[1].memory,
				lb->effectiveGelOffset1 * sizeof(s_wallPerGelData), sizeof(s_wallPerGelData) * lb->eDatas[0].unitWalls.size(), 0, (void**)&gelData);
			for (int i = 0; i < size; i++)
			{
				gelData[i].data.x = gelTypeToIndex(lb->eDatas[0].unitWalls[i].gelType);
				gelData[i].data.y = lb->eDatas[0].unitWalls[i].gelAmount;
			}
			vkUnmapMemory(ThinDrawer::object->logicalDevice, ThinDrawer::object->wallGel->uniformBuffers[1].memory);

			vkMapMemory(ThinDrawer::object->logicalDevice, ThinDrawer::object->wallGel->uniformBuffers[1].memory,
				lb->effectiveGelOffset2 * sizeof(s_wallPerGelData), sizeof(s_wallPerGelData) * lb->eDatas[1].unitWalls.size(), 0, (void**)&gelData);
			for (int i = 0; i < size; i++)
			{
				gelData[i].data.x = gelTypeToIndex(lb->eDatas[1].unitWalls[i].gelType);
				gelData[i].data.y = lb->eDatas[1].unitWalls[i].gelAmount;
			}
			vkUnmapMemory(ThinDrawer::object->logicalDevice, ThinDrawer::object->wallGel->uniformBuffers[1].memory);

		}

		portalConnection* conn;
		Portal* otherPortal;

		if (isPortal)
		{
			conn = portal->connections[0][0];
			otherPortal = conn->portal2;

			lb->outBody = otherPortal->body;

			lb->frames.p1v1 = portal->dir;
			lb->frames.p1v2 = conn->isReversed ? -portal->rotDir : portal->rotDir;
			lb->frames.p2v1 = otherPortal->dir;
			lb->frames.p2v2 = otherPortal->rotDir;

			perTickChecks.push_back(lb);

			lb->reversed = !conn->isReversed;
		}
		else
		{
			lb->outBody = nullptr;
		}

		lb->renderLightPositions.clear();

		for (; isPortal;)
		{
			b2Vec2 endPos = raySendPos + minLen * rayDir;

			b2Vec2 midP = PortalWorld::getRelativePortalPosition_W(endPos, conn);

			raySendPos = midP - 0.2f * otherPortal->dir;
			rayDir = otherPortal->dir;
			portal = sendRays(400.0f, raySendPos, rayDir, 1, &minLen);

			isPortal = portal != nullptr;

			lb->renderLightPositions.push_back({ midP, raySendPos + minLen * rayDir });

			lb->reqMapCount = ThinDrawer::object->swapChain->imageCount;
			lb->totalSize += minLen - 0.2f;
		}

		int newSize = std::ceil(lb->totalSize * 10.0f);
		if (newSize != lb->eDatas[0].unitWalls.size())
		{
			int oldSize = lb->eDatas[0].unitWalls.size();

			lb->eDatas[0].unitWalls.resize(newSize);
			lb->eDatas[1].unitWalls.resize(newSize);

			int diff = newSize - oldSize;
			if (diff > 0)
			{
				s_wallPerGelData* gelData;
				vkMapMemory(ThinDrawer::object->logicalDevice, ThinDrawer::object->wallGel->uniformBuffers[1].memory,
					(lb->effectiveGelOffset1 + oldSize) * sizeof(s_wallPerGelData), sizeof(s_wallPerGelData) * diff, 0, (void**)&gelData);
				for (int i = 0; i < diff; i++)
				{
					gelData[i].data.y = 0.0f;
				}
				vkUnmapMemory(ThinDrawer::object->logicalDevice, ThinDrawer::object->wallGel->uniformBuffers[1].memory);

				vkMapMemory(ThinDrawer::object->logicalDevice, ThinDrawer::object->wallGel->uniformBuffers[1].memory,
					(lb->effectiveGelOffset2 + oldSize) * sizeof(s_wallPerGelData), sizeof(s_wallPerGelData) * diff, 0, (void**)&gelData);
				for (int i = 0; i < diff; i++)
				{
					gelData[i].data.y = 0.0f;
				}
				vkUnmapMemory(ThinDrawer::object->logicalDevice, ThinDrawer::object->wallGel->uniformBuffers[1].memory);
			}
		}

		if (lightRay.closestFixture)
		{
			b2Vec2 endPos = raySendPos + minLen * rayDir;
			if ((lb->capBody->GetPosition() - endPos).LengthSquared() > 0.001f)
			{
				float angle = atan2(rayDir.y, rayDir.x);
				lb->capBody->SetTransform(endPos, angle);
			}

			lb->capBody->SetLinearVelocity(lightRay.closestFixture->GetBody()->GetLinearVelocity());
		}
		else if ((lb->capBody->GetPosition() - lb->lightPos).LengthSquared() > 0.001f)
		{
			lb->capBody->SetTransform(lb->lightPos, -b2_pi * 0.5f);
			lb->capBody->SetLinearVelocity(b2Vec2(0.0f, 0.0f));
		}

		if (lb->extraRenderIndex > -1)
		{
			extraIndices[lb->extraRenderIndex] = lb->renderLightPositions.size();
		}
	}
}

void LightBridge::perTickUpdate()
{
	for (int i = 0; i < perTickChecks.size(); i++)
	{
		LightBridge* lb = perTickChecks[i];

		if (lb->renderLightPositions.size() >= lb->bodies.size())
		{
			float offset = lb->lastLen;
			for (int j = 1; j < lb->bodies.size(); j++)
			{
				float len = (lb->renderLightPositions[j - 1].p1 - lb->renderLightPositions[j - 1].p0).Length();
				offset += len;
			}

			for (int j = lb->bodies.size() - 1; j < lb->renderLightPositions.size(); j++)
			{
				fixtureData* fData = (fixtureData*)malloc(sizeof(fixtureData) * 2 + sizeof(s_edgeLocalData) * 2);
				s_edgeLocalData* eData = (s_edgeLocalData*)&fData[2];

				fData[0].extraData = nullptr; fData[1].extraData = nullptr;
				eData[0].prevNextFixtures[0] = nullptr; eData[0].prevNextFixtures[1] = nullptr;
				eData[1].prevNextFixtures[0] = nullptr; eData[1].prevNextFixtures[1] = nullptr;

				fData[0].data = (void*)&eData[0]; fData[0].type = F_LIGHT_BRIDGE;
				fData[1].data = (void*)&eData[1]; fData[1].type = F_LIGHT_BRIDGE;

				eData[0].globalData = &lb->eDatas[0];
				eData[1].globalData = &lb->eDatas[1];

				eData[0].globalRange[0] = std::round(offset * 10.0f);
				eData[0].globalRange[1] = lb->eDatas[0].unitWalls.size() - 1;

				eData[1].globalRange[0] = eData[0].globalRange[0];
				eData[1].globalRange[1] = eData[0].globalRange[1];

				float endAdd = j == lb->renderLightPositions.size() - 1 ? -0.15f : 0.15f;

				s_lightRay lRay = lb->renderLightPositions[j];
				b2Vec2 diff = lRay.p1 - lRay.p0;
				float len = diff.Normalize();

				b2BodyDef bDef;
				if (lb->endBody && lb->outBody && (lb->endBody->GetType() == b2_kinematicBody || lb->outBody->GetType() == b2_kinematicBody))
					bDef.type = b2_kinematicBody;
				else bDef.type = b2_staticBody;
				b2Body* body = Game::object->pWorld->CreateBody(&bDef);

				lb->bodies.push_back(body);

				b2EdgeShape shape;

				float reverser = lb->reversed && (j % 2 == 0) ? -1.0f : 1.0f;

				b2Vec2 p1 = lRay.p0 - reverser * 0.5f * lightThick * lb->frames.p2v2 - 0.15f * diff;
				b2Vec2 p2 = p1 + (len + 0.15f + endAdd) * diff;

				shape.SetTwoSided(p1, p2);
				b2Fixture* fix = body->CreateFixture(&shape, 0.0f);
				fix->GetUserData().pointer = (uintptr_t)&fData[0];

				p1 = p1 + reverser * lightThick * lb->frames.p2v2;
				p2 = p2 + reverser * lightThick * lb->frames.p2v2;

				shape.SetTwoSided(p1, p2);
				fix = body->CreateFixture(&shape, 0.0f);
				fix->GetUserData().pointer = (uintptr_t)&fData[1];

				offset += len;
			}
		}
		else if (lb->renderLightPositions.size() + 1 < lb->bodies.size())
		{
			for (int j = lb->renderLightPositions.size(); j < lb->bodies.size(); j++)
			{
				b2Fixture* fix = lb->bodies[j]->GetFixtureList();
				fixtureData* fData = (fixtureData*)(fix->GetUserData().pointer - sizeof(fixtureData));
				free(fData);
				fix->GetUserData().pointer = 0; fix->GetNext()->GetUserData().pointer = 0;
				Game::object->pWorld->DestroyBody(lb->bodies[j]);
			}

			lb->bodies.resize(lb->renderLightPositions.size());
		}

		b2Vec2 s0 = lb->endBody->GetLinearVelocity();

		float fwdSpeed = b2Dot(lb->frames.p1v1, s0);
		float rightSpeed = b2Dot(lb->frames.p1v2, s0);

		b2Vec2 speed = b2Vec2(0.0f, 0.0f);
		b2Vec2 speedAddition = fwdSpeed * lb->frames.p2v1 + rightSpeed * lb->frames.p2v2 + lb->outBody->GetLinearVelocity();
		float offset = lb->lastLen;
		for (int j = 1; j < lb->bodies.size(); j++)
		{
			b2Body* body = lb->bodies[j];

			speed += speedAddition;
			body->SetLinearVelocity(speed);

			float endAdd = j == lb->bodies.size() - 1 ? -0.15f : 0.15f;

			s_lightRay lRay = lb->renderLightPositions[j - 1];

			b2EdgeShape* eShape = (b2EdgeShape*)body->GetFixtureList()->GetShape();
			b2Vec2 p0 = body->GetWorldPoint(eShape->m_vertex1);
			b2Vec2 p1 = body->GetWorldPoint(eShape->m_vertex2);

			float reverser = lb->reversed && (j % 2 == 1) ? -1.0f : 1.0f;

			b2Vec2 pCheck0 = lRay.p0 - 0.15f * lb->frames.p2v1 + reverser * 0.5f * lightThick * lb->frames.p2v2;
			b2Vec2 pCheck1 = lRay.p1 + endAdd * lb->frames.p2v1 + reverser * 0.5f * lightThick * lb->frames.p2v2;

			float diff0 = (pCheck0 - p0).LengthSquared();
			float diff1 = (pCheck1 - p1).LengthSquared();

			if ((diff0 > 0.01f || diff1 > 0.01f))
			{
				fixtureData* fData = (fixtureData*)(body->GetFixtureList()->GetUserData().pointer - sizeof(fixtureData));
				s_edgeLocalData* eData = (s_edgeLocalData*)fData->data;

				eData[0].globalRange[0] = std::round((offset - 0.15f) * 10.0f);
				eData[1].globalRange[0] = eData[0].globalRange[0];

				eData[0].globalRange[1] = lb->eDatas[0].unitWalls.size() - 1; eData[1].globalRange[1] = eData[0].globalRange[1];

				body->DestroyFixture(body->GetFixtureList());
				body->DestroyFixture(body->GetFixtureList());

				b2Vec2 diff = lRay.p1 - lRay.p0;
				float len = diff.Normalize();

				b2EdgeShape shape;

				b2Vec2 p1 = lRay.p0 - reverser * 0.5f * lightThick * lb->frames.p2v2 - 0.15f * diff - body->GetPosition();
				b2Vec2 p2 = p1 + (len + 0.15f + endAdd) * diff;

				shape.SetTwoSided(p1, p2);
				b2Fixture* fix = body->CreateFixture(&shape, 0.0f);
				fix->GetUserData().pointer = (uintptr_t)&fData[0];

				p1 = p1 + reverser * lightThick * lb->frames.p2v2;
				p2 = p2 + reverser * lightThick * lb->frames.p2v2;

				shape.SetTwoSided(p1, p2);
				fix = body->CreateFixture(&shape, 0.0f);
				fix->GetUserData().pointer = (uintptr_t)&fData[1];
			}

			offset += (lRay.p1 - lRay.p0).Length();
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

	//ThinDrawer::object->registerDebugLine(glm::vec4(rayStart.x, rayStart.y, endPos.x, endPos.y), glm::vec4(0.0f, 0.0f, 1.0f, 1.0f), 0.1f, 0.5f);

	Game::object->pWorld->RayCast(this, rayStart, rayStart + rayLength * dirVec);

	endPos = rayStart + rayLength * minFraction * dirVec;

	//ThinDrawer::object->registerDebugLine(glm::vec4(rayStart.x, rayStart.y, endPos.x, endPos.y), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), 0.02f, 0.5f);
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